#ifdef BUF_STATIC_LIBRARY
#	define BUF_API
#elif BUF_DYNAMIC_LIBRARY
#	define BUF_API __declspec(dllimport)
#elif BUILD_DYNAMIC_LIBRARY
#	define BUF_API __declspec(dllexport)
#else
#	define BUF_API
#endif
