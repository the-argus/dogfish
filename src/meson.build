# see https://mesonbuild.com/Builtin-options.html#details-for-buildtype
project('lungfish', 'c', default_options: ['default_library=static', 'buildtype=debug', 'warning_level=0'])

raylib = dependency('raylib')

ode = dependency('ode')

# add_global_arguments('-fsanitize=address', language : 'c')
# add_global_arguments('-fno-omit-frame-pointer', language : 'c')

source_file_names = [
  'main.c',
  'input.c',
  'camera_manager.c',
  'fps_camera.c',
  'physics.c',
  'debug.c'
]

sources = []

foreach file : source_file_names
  sources += files(file)
endforeach

my_headers = declare_dependency(include_directories : include_directories('include', '.'))

link_args = [ '-lm', '-lstdc++' ]

# additional_flags = [ '-fsanitize=address', '-fno-omit-frame-pointer' ]
additional_flags = [ ]
foreach flag : additional_flags
  link_args += flag
endforeach

executable('lungfish', sources, dependencies: [ ode, raylib, my_headers ], link_args: link_args)
