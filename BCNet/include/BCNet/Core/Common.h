#pragma once

// Shouldn't be included directly by the application, but needs to be here so the application can import the symbols from the DLL.

#define DEFAULT_SERVER_PORT 5456

#ifdef BCNET_API_STATIC // Static linking
#define BCNET_API 
#else

// DLL export/import macros
// TODO: GCC/Clang supported macros.
#ifdef BCNET_API_EXPORTS
#define BCNET_API __declspec(dllexport) // MSVC specific.
#else
#define BCNET_API __declspec(dllimport) // MSVC Specific
#endif

#endif
