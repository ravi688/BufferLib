#pragma once

/*
	Switches:
		(NOTE: Automatically defined by the build system when building shared or static libraries)
		Development Switches:
			BUF_BUILD_STATIC_LIBRARY
			BUF_BUILD_DYNAMIC_LIBRARY
		(NOTE: Need to be defined by a dependent project before including any of this library's headers)
		UserCode Switches:
			BUF_USE_STATIC_LIBRARY
			BUF_USER_DYNAMIC_LIBRARY
*/

#if (defined _WIN32 || defined __CYGWIN__) && defined(__GNUC__)
#	define BUF_IMPORT_API __declspec(dllimport)
#	define BUF_EXPORT_API __declspec(dllexport)
#else
#	define BUF_IMPORT_API __attribute__((visibility("default")))
#	define BUF_EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef BUF_BUILD_STATIC_LIBRARY
#	define BUF_API
#elif defined(BUF_BUILD_DYNAMIC_LIBRARY)
#	define BUF_API BUF_EXPORT_API
#elif defined(BUF_USE_DYNAMIC_LIBRARY)
#	define BUF_API BUF_IMPORT_API
#elif defined(BUF_USE_STATIC_LIBRARY)
#	define BUF_API
#else
#	define BUF_API
#endif


#if !defined(BUF_RELEASE) && !defined(BUF_DEBUG)
#	warning "None of BUF_RELEASE && BUF_DEBUG is defined; using BUF_DEBUG"
#	define BUF_DEBUG
#endif
