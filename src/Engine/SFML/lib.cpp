//
// Created by PinkySmile on 10/01/2026.
//

#include "Resources/MSVCMacros.hpp"

#ifdef __linux__
#include <X11/Xlib.h>

void MYDLL_API libraryInit()
{
	XInitThreads();
}
#else
void MYDLL_API libraryInit()
{
}
#endif