#include "ccolourpicker.hpp"
#include <windows.h>  
#include <stdlib.h>

#include "gui.hpp"

HRESULT CColourPicker::CreateColourPicker()
{
	return GUI::CreateGUI(this, Value, hParent);
}
