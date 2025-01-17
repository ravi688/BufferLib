# ------------------------------ DOCUMENTATION ---------------------------------
# Release build
# -------------------------
# $ meson setup --wipe <builddir> # wipe the build artifacts (like object files)
# $ meson setup <builddir> --reconfigure --buildtype=release # reconfigure the build directory for release build
# $ meson compile -C <builddir> # compile the project
#
# Debug build
# -------------------------
# $ meson setup --wipe <buildir> # wipe the build artifacts (like object files)
# $ meson setup <builddir> --reconfigure --buildtype=release # reconfigure the build directory for debug build
# $ meson compile -C <builddir> # compile the project
#
# Static Library
# -------------------------
# $ meson setup --wipe <buildir> # wipe the build artifacts (like object files)
# # NOTE: --buildtype=release or --buildtype=debug options can be added here  
# $ meson setup -C <builddir> --reconfigure --default-library=static # reconfigure the build directory for static library
# $ meson compile -C <builddir> # compile the project
# $ meson install -C <builddir> # install the static library
#
# Shared Library
# -------------------------
# $ meson setup --wipe <buildir> # whipe the build artifacts (like object files)
# # NOTE: --buildtype=release or --buildtype=debug options can be added here
# $ meson setup -C <builddir> --reconfigure --default-library=shared # reconfigure the build directory for shared library
# $ meson compile -C <builddir> # compile the project
# $ meson install -C <builddir> # install the shared library
#
# Artifact Installation Directories
# ---------------------------------
# Headers: /include/<ProjectNameInSmallCase>
# Static Libraries: /lib/lib<ProjectNameInSmallCase>.a-
# Shared Libraries: /bin/lib<ProjectNameInSmallCase>.dll
# PkgConfig (.pc) for static library: $PKG_CONFIG_PATH/<ProjectNameInSmallCase>_static.pc
# PkgConfig (.pc) for shared library: $PKG_CONFIG_PATH/<ProjectNameInSmallCase>_shared.pc
#
# -------------------------------- PROJECT CONFIGS -----------------------------

project('BufferLib', 'c', 'cpp',
  version : '1.0.0',
  meson_version: '>=1.1',
  default_options : [
    'warning_level=3',
    'buildtype=debug',
    'c_std=c17',
    'cpp_std=c++20'
  ]
)

# Release Build Defines
release_defines = [
  '-DBUF_RELEASE' 
]

# Debug Build Defines
debug_defines = [
  '-DBUF_DEBUG'
]

# Shared Library Build Defines
shared_library_build_defines = [
  '-DBUF_BUILD_DYNAMIC_LIBRARY'
]

# Shared Library Use Defines
shared_library_use_defines = [
  '-DBUF_USE_DYNAMIC_LIBRARY'
]

# Static Library Build Defines
static_library_build_defines = [
  '-DBUF_BUILD_STATIC_LIBRARY'
]

# Static Library Use Defines
static_library_use_defines = [
  '-DBUF_USE_STATIC_LIBRARY'
]

# Source files (excluding main.c)
lib_sources = files(
  'source/buffer.c',
  'source/buffer_test.c',
  'source/main.c'
)

# Include directories
lc_project_name = get_option('project_name').to_lower()
inc = include_directories('include', 'include/' + lc_project_name)

# Main executable source
main_source = files('source/main.c')

# Whether to install executable?
is_install_executable = false

# Dependencies
dependencies = [
  dependency('calltrace', required : true, fallback: ['calltrace', 'calltrace_dep'])
]

# ------------------------------ INTERNALS ---------------------------------------

# Compiler configuration
add_project_arguments('-m64', language : 'c')
add_project_arguments('-m64', language : 'cpp')
# Linker configuration
add_project_link_arguments('-m64', language : 'c')
add_project_link_arguments('-m64', language : 'cpp')

# Build type specific defines
build_mode_defines = []
if get_option('buildtype') == 'release'
  add_project_arguments(release_defines, language : 'c')
  add_project_arguments(release_defines, language : 'cpp')
  build_mode_defines += release_defines
else
  add_project_arguments(debug_defines, language : 'c')
  add_project_arguments(debug_defines, language : 'cpp')
  build_mode_defines += debug_defines
endif

# Static Library
static_lib = static_library('bufferlib_static',
  lib_sources,
  dependencies: dependencies,
  include_directories : inc,
  install : true,
  install_dir : get_option('libdir')/lc_project_name,
  c_args : static_library_build_defines,
  cpp_args : static_library_build_defines,
  gnu_symbol_visibility: 'hidden'
)

bufferlib_dep = declare_dependency(
    link_with: static_lib,
    dependencies: dependencies,
    include_directories: inc,
    compile_args: build_mode_defines
)

# Shared Library
shared_lib = shared_library('bufferlib_shared',
  lib_sources,
  dependencies: dependencies,
  include_directories : inc,
  install : true,
  install_dir : get_option('libdir')/lc_project_name,
  c_args : shared_library_build_defines,
  cpp_args : shared_library_build_defines,
  gnu_symbol_visibility: 'hidden'
)

# Main executable
exe = executable(lc_project_name,
  main_source,
  dependencies : [bufferlib_dep],
  include_directories : inc,
  install : is_install_executable
)

# Header installation
install_subdir('include/bufferlib', install_dir : get_option('includedir'))

# pkg-config package installation
python = find_program('python')

# Try PKG_CONFIG_PATH first, typicallly it succeeds on MINGW64 (MSYS2)
result = run_command(python, '-c', 'import os; print(os.environ["PKG_CONFIG_PATH"])', check : false)
if result.returncode() == 0
  str = result.stdout()
  # Unix
  if str.startswith('/')
    pkgconfig_install_path = str.replace(';', ':').split(':')[0]
  # Windows
  else
    pkgconfig_install_path = str.split(';')[0]
  endif
endif
if pkgconfig_install_path == ''
  # Otherwise use pkg-config to query its lookup directories
  message('PKG_CONFIG_PATH seems to be empty, trying another method')
  result = run_command('pkg-config', '--variable', 'pc_path', 'pkg-config', check : false)
  if result.returncode() == 0
    str = result.stdout()
    if str.startswith('/')
      pkgconfig_install_path = str.replace(';', ':').split(':')[0]
    else
      pkgconfig_install_path = str.split(';')[0]
    endif
  # Finally if the above attempts fail, use 'libdir' value
  else
    pkgconfig_install_path = get_option('libdir')
  endif
endif
message('pkg config path: ' + pkgconfig_install_path)

pkgmod = import('pkgconfig')
# Generate .pc file for static library 
pkgmod.generate(static_lib,
    name: get_option('project_name'),
    description: 'A function call trace library',
    filebase: 'bufferlib_static',
    install_dir: pkgconfig_install_path,
    extra_cflags: static_library_use_defines + build_mode_defines
)
# Generate .pc file for shared library
pkgmod.generate(shared_lib,
    name: get_option('project_name'),
    description: 'A function call trace library',
    filebase: 'bufferlib_shared',
    install_dir: pkgconfig_install_path,
    extra_cflags: shared_library_use_defines + build_mode_defines
)

bufferlib_dep = declare_dependency(
    link_with: static_lib,
    include_directories: inc,
    compile_args: static_library_use_defines + build_mode_defines
)
