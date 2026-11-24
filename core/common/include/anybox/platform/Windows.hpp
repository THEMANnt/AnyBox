#ifndef ANYBOX_PLATFORM_WINDOWS_HPP
#define ANYBOX_PLATFORM_WINDOWS_HPP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#error This implementation requires Windows
#endif

#endif
