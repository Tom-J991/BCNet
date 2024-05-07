#pragma once

#define DEFAULT_SERVER_PORT 5456

#ifdef BCNET_API_STATIC // Static linking
#define BCNET_API 
#else

// DLL stuff
// TODO: GCC/Clang supported macros.
#ifdef BCNET_API_EXPORTS
#define BCNET_API __declspec(dllexport) // MSVC Specific
#else
#define BCNET_API __declspec(dllimport) // MSVC Specific
#endif

#endif

#ifdef _DEBUG
#define BCNET_API_DEBUG
#else
#define BCNET_API_RELEASE
#endif
