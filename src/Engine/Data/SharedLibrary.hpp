//
// Created by PinkySmile on 11/10/2026.
//

#ifndef MIXRTS_SHAREDLIBRARY_HPP
#define MIXRTS_SHAREDLIBRARY_HPP


#ifdef _WIN32
#include <windows.h>
#define dlopen(path, _) LoadLibraryA(path)
#define dlsym(sl, name) reinterpret_cast<void *>(GetProcAddress(sl, name))
#define dlclose(sl) FreeLibrary(sl)
#define SL_EXPORT extern "C" __declspec(dllexport)
typedef HMODULE SharedLibrary;

inline std::string dlerror()
{
	DWORD err = GetLastError();
	wchar_t *s;
	auto chars = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s,
		0,
		NULL
	);

	std::stringstream stream;

	stream << "GetLastError " << err;
	if (!chars)
		return stream.str();
	stream << ": ";
	for (int i = 0; s[i]; i++)
		stream << static_cast<char>(s[i]);
	LocalFree(s);
	return stream.str();
}
#else
#include <dlfcn.h>
typedef void *SharedLibrary;
#define SL_EXPORT extern "C"
#endif

#define ABI_VERSION 1U
#define SL_DECLARE_VERSION SL_EXPORT unsigned _ABI_VERSION; unsigned _ABI_VERSION = ABI_VERSION


#endif //MIXRTS_SHAREDLIBRARY_HPP
