
#ifdef BUILD_DYNAMIC_LIBRARY
#	define BUF_API __declspec(dllexport)
#elif BUF_DYNAMIC_LIBRARY
#	define BUF_API __declspec(dllimport)
#else
#	define BUF_API
#endif
