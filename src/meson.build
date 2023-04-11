# see https://mesonbuild.com/Builtin-options.html#details-for-buildtype
project('dogfish', 'c', default_options: ['default_library=static', 'buildtype=debugoptimized', 'warning_level=3'])

cc = meson.get_compiler('c')

ode = dependency('ode', required: false)
raylib = dependency('raylib', required: false)

windows_lib = meson.source_root() + '\\windows\\lib'
windows_include = 'windows/include'

# backup library locations
windows_linkflags = [ '-L' + windows_lib, '-lode_single', '-lwinmm' ]

if not raylib.found()
  raylib = declare_dependency(
    dependencies: cc.find_library('raylib', dirs: [windows_lib]),
    include_directories: include_directories(windows_include))
endif

# add_global_arguments('-fsanitize=address', language : 'c')
# add_global_arguments('-fno-omit-frame-pointer', language : 'c')

source_file_names = [
  'main.c',
  'input.c',
  'camera_manager.c',
  'fps_camera.c',
  'physics.c',
  'debug.c',
  'object_structure',
  'dynarray_implementation.c',
  'skybox.c'
]

sources = []

foreach file : source_file_names
  sources += files(file)
endforeach

includes = ['include', '.']
link_args = [ '-lm', '-L../lib' ]

if build_machine.system() == 'windows' or build_machine.system() == 'cygwin'
  includes += windows_include
else
  link_args += '-lstdc++'
endif

my_headers = declare_dependency(include_directories : include_directories(includes))

# additional_flags = [ '-fsanitize=address', '-fno-omit-frame-pointer' ]
additional_flags = [ ]
foreach flag : additional_flags
  link_args += flag
endforeach

if build_machine.system() == 'windows' or build_machine.system() == 'cygwsn'
  link_args += windows_linkflags
endif

executable('dogfish', sources, dependencies: [ ode, raylib, my_headers ], link_args: link_args)
