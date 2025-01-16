# BufferLib
- BufferLib is a small library for managing memory buffers

## Building and Installing
### Release build
```
 $ meson setup --wipe <builddir> # wipe the build artifacts (like object files)
 $ meson setup <builddir> --reconfigure --buildtype=release # reconfigure the build directory for release build
 $ meson compile -C <builddir> # compile the project
```
### Debug build
```
 $ meson setup --wipe <buildir> # wipe the build artifacts (like object files)
 $ meson setup <builddir> --reconfigure --buildtype=release # reconfigure the build directory for debug build
 $ meson compile -C <builddir> # compile the project
```

### Static Library
```
 $ meson setup --wipe <buildir> # wipe the build artifacts (like object files)
 # NOTE: --buildtype=release or --buildtype=debug options can be added here  
 $ meson setup -C <builddir> --reconfigure --default-library=static # reconfigure the build directory for static library
 $ meson compile -C <builddir> # compile the project
 $ meson install -C <builddir> # install the static library
```
### Shared Library
```
 $ meson setup --wipe <buildir> # whipe the build artifacts (like object files)
 # NOTE: --buildtype=release or --buildtype=debug options can be added here
 $ meson setup -C <builddir> --reconfigure --default-library=shared # reconfigure the build directory for shared library
 $ meson compile -C <builddir> # compile the project
 $ meson install -C <builddir> # install the shared library
```
### Artifact Installation Directories
- Headers: /include/<ProjectNameInSmallCase>
- Static Libraries: /lib/lib<ProjectNameInSmallCase>.a-
- Shared Libraries: /bin/lib<ProjectNameInSmallCase>.dll
- PkgConfig (.pc) for static library: $PKG_CONFIG_PATH/<ProjectNameInSmallCase>_static.pc
- PkgConfig (.pc) for shared library: $PKG_CONFIG_PATH/<ProjectNameInSmallCase>_shared.pc

<a href="https://www.buymeacoffee.com/raviprakashsingh" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>
## Documentation

**Types**
- BUFFER --> The Buffer Object which contains all the information such as its size, strides, count, and capacity
- pBUFFER --> Just a typedef of BUFFER*


**Two ways**
- Bind -> Perform Operations -> UnBind
- Perform Operations passing the object (buffer)

Example:
```C
#include <bufferlib/buffer.h>
#include <stdint.h>

int main(int argc, const char* argv[])
{
  buffer_t values = buf_new(int32_t);
  buf_push_auto(&values, 4);
  buf_push_auto(&values, 100);
  buf_push_auto(&values, 101);
  buf_traverse_elements(&values, 0, buf_get_element_count(&values) - 1, buf_s32_print, NULL); 
  buf_free(&values);
  return 0;
}
```

### About Me
[Youtube](https://www.youtube.com/channel/UCWe_os3p4z3DBnQ4B5DUTfw/videos) </br>
[Github](https://github.com/ravi688) </br>
[LinkedIn](https://www.linkedin.com/in/ravi-prakash-095a271a8/) </br>
