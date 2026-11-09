//
// Created by PinkySmile on 10/01/2026.
//

#ifdef __linux__
#include <X11/Xlib.h>

void libraryInit()
{
	XInitThreads();
}
#else
void libraryInit()
{
}
#endif