#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <spdlog/common.h>
#include <string_view>
#include <unordered_map>

#include "../util/null_terminated_string_view.hpp"
#include "optionaltaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"
#include "windowfilter.hpp"


#ifdef _TRANSLUCENTTB_EXE
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <rapidjson/encodedstream.h>
#include <rapidjson/error/error.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <wil/resource.h>

#include "../appinfo.hpp"
#include "../../ProgramLog/error/rapidjson.hpp"
#include "../../ProgramLog/error/std.hpp"
#endif


enum class PeekBehavior {
	AlwaysShow = 0,                   // Always show the button
	WindowMaximisedOnMainMonitor = 1, // Show when a window is maximised on the main monitor
	WindowMaximisedOnAnyMonitor = 2,  // Show when a window is maximised on any monitor
	DesktopIsForegroundWindow = 3,    // Show when the desktop is the foreground window
	AlwaysHide = 4                    // Always hide the button
};

class Config {
public:
	// Appearances
	TaskbarAppearance DesktopAppearance;
	OptionalTaskbarAppearance VisibleWindowAppearance;
	OptionalTaskbarAppearance MaximisedWindowAppearance;
	OptionalTaskbarAppearance StartOpenedAppearance;
	OptionalTaskbarAppearance CortanaOpenedAppearance;
	OptionalTaskbarAppearance TimelineOpenedAppearance;

	// Peek
	PeekBehavior Peek;
	bool UseRegularAppearanceWhenPeeking;

	// Advanced
	WindowFilter Whitelist;
	WindowFilter Blacklist;
	bool HideTray;
	bool DisableSaving;
	spdlog::level::level_enum LogVerbosity;

	// Default-init with default settings
	inline Config() noexcept :
		DesktopAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0 },
		VisibleWindowAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0, false },
		MaximisedWindowAppearance { ACCENT_ENABLE_BLURBEHIND, 0xAA000000, true },
		StartOpenedAppearance { ACCENT_NORMAL, 0, true },
		CortanaOpenedAppearance { ACCENT_NORMAL, 0, true },
		TimelineOpenedAppearance { ACCENT_NORMAL, 0, true },
		Peek(PeekBehavior::WindowMaximisedOnMainMonitor),
		UseRegularAppearanceWhenPeeking(true),
		Whitelist(),
		Blacklist(),
		HideTray(false),
		DisableSaving(false),
#ifdef _DEBUG
		LogVerbosity(spdlog::level::debug)
#else
		LogVerbosity(spdlog::level::warn)
#endif
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Serialize(writer, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Serialize(writer, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Serialize(writer, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Serialize(writer, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Serialize(writer, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Serialize(writer, Peek, PEEK_KEY, s_PeekMap);
		RapidJSONHelper::Serialize(writer, UseRegularAppearanceWhenPeeking, REGULAR_ON_PEEK_KEY);
		RapidJSONHelper::Serialize(writer, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Serialize(writer, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Serialize(writer, HideTray, TRAY_KEY);
		RapidJSONHelper::Serialize(writer, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Serialize(writer, LogVerbosity, LOG_KEY, s_LogMap);
	}

	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		RapidJSONHelper::Deserialize(val, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Deserialize(val, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Deserialize(val, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Deserialize(val, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Deserialize(val, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Deserialize(val, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Deserialize(val, Peek, PEEK_KEY, s_PeekMap);
		RapidJSONHelper::Deserialize(val, UseRegularAppearanceWhenPeeking, REGULAR_ON_PEEK_KEY);
		RapidJSONHelper::Deserialize(val, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Deserialize(val, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Deserialize(val, HideTray, TRAY_KEY);
		RapidJSONHelper::Deserialize(val, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Deserialize(val, LogVerbosity, LOG_KEY, s_LogMap);
	}

#ifdef _TRANSLUCENTTB_EXE
	inline void Save(const std::filesystem::path &file, bool ignoreDisabledSaving = false)
	{
		if (ignoreDisabledSaving || !DisableSaving)
		{
			wil::unique_file pfile;
			const errno_t err = _wfopen_s(pfile.put(), file.c_str(), L"wb");
			if (err == 0)
			{
				using namespace rapidjson;

				char buffer[256];
				FileWriteStream filestream(pfile.get(), buffer, std::size(buffer));

				using OutputStream = EncodedOutputStream<UTF16LE<>, FileWriteStream>;
				OutputStream out(filestream, true);

				static constexpr std::wstring_view comment = L"// See https://" APP_NAME L".github.io/config for more info\n";
				for (const wchar_t c : comment)
				{
					out.Put(c);
				}

				PrettyWriter<OutputStream, UTF16LE<>, UTF16LE<>> writer(out);

				writer.StartObject();
				Serialize(writer);
				writer.EndObject();

				writer.Flush();
			}
			else
			{
				ErrnoTHandle(err, spdlog::level::err, L"Failed to save configuration!");
			}
		}
	}

	inline static Config Load(const std::filesystem::path &file)
	{
		// This check is so that if the file gets deleted for whatever reason while the app is running, default configuration gets restored immediatly.
		if (std::filesystem::is_regular_file(file))
		{
			wil::unique_file pfile;
			const errno_t err = _wfopen_s(pfile.put(), file.c_str(), L"rb");
			if (err == 0)
			{
				using namespace rapidjson;

				char buffer[256];
				FileReadStream filestream(pfile.get(), buffer, std::size(buffer));

				AutoUTFInputStream<uint32_t, FileReadStream> in(filestream);

				GenericDocument<UTF16LE<>> doc;
				if (ParseResult result = doc.ParseStream<kParseCommentsFlag, AutoUTF<uint32_t>>(in))
				{
					Config cfg;
					// TODO: exception throwing & catching, handle std::system_error from FilenameSet
					cfg.Deserialize(doc);
					return cfg;
				}
				else
				{
					ParseErrorCodeHandle(result.Code(), spdlog::level::err, L"Failed to parse configuration!");
				}
			}
			else
			{
				ErrnoTHandle(err, spdlog::level::err, L"Failed to load configuration!");
			}
		}

		return { };
	}
#endif

private:
	inline static const std::unordered_map<PeekBehavior, std::wstring_view> s_PeekMap = {
		{ PeekBehavior::AlwaysHide,                   L"never"                                 },
		{ PeekBehavior::WindowMaximisedOnMainMonitor, L"when_maximised_window_on_main_monitor" },
		{ PeekBehavior::WindowMaximisedOnAnyMonitor,  L"when_maximised_window_on_any_monitor"  },
		{ PeekBehavior::DesktopIsForegroundWindow,    L"when_desktop_is_foreground_window"     },
		{ PeekBehavior::AlwaysShow,                   L"always"                                }
	};

	inline static const std::unordered_map<spdlog::level::level_enum, std::wstring_view> s_LogMap = {
		{ spdlog::level::debug, L"debug"   },
		{ spdlog::level::info,  L"info"    },
		{ spdlog::level::warn,  L"warning" },
		{ spdlog::level::err,   L"error"   },
		{ spdlog::level::off,   L"off"     }
	};

	static constexpr Util::null_terminated_wstring_view DESKTOP_KEY = L"desktop_appearance";
	static constexpr Util::null_terminated_wstring_view VISIBLE_KEY = L"visible_window_appearance";
	static constexpr Util::null_terminated_wstring_view MAXIMISED_KEY = L"maximised_window_appearance";
	static constexpr Util::null_terminated_wstring_view START_KEY = L"start_opened_appearance";
	static constexpr Util::null_terminated_wstring_view CORTANA_KEY = L"cortana_opened_appearance";
	static constexpr Util::null_terminated_wstring_view TIMELINE_KEY = L"timeline_opened_appearance";
	static constexpr Util::null_terminated_wstring_view PEEK_KEY = L"show_peek_button";
	static constexpr Util::null_terminated_wstring_view REGULAR_ON_PEEK_KEY = L"regular_appearance_when_peeking";
	static constexpr Util::null_terminated_wstring_view WHITELIST_KEY = L"whitelist";
	static constexpr Util::null_terminated_wstring_view BLACKLIST_KEY = L"blacklist";
	static constexpr Util::null_terminated_wstring_view TRAY_KEY = L"hide_tray";
	static constexpr Util::null_terminated_wstring_view SAVING_KEY = L"disable_saving";
	static constexpr Util::null_terminated_wstring_view LOG_KEY = L"verbosity";
};
