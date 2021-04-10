#pragma once

#include <wdm.h>

PVOID get_module_symbol_address(wchar_t* module_name, char* symbol_name);