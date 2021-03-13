#include <string>
#include <windows.h>
#include <iostream>

#include "basewin.h"

std::wstring s2ws(const std::string& s);
LPCWSTR stringToLPCWSTR(std::string str);