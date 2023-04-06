# see https://mesonbuild.com/Builtin-options.html#details-for-buildtype
project('lungfish', 'c', default_options: ['default_library=static', 'buildtype=debugoptimized', 'warning_level=0'])

raylib = dependency('raylib')

ode = dependency('ode')

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

executable('lungfish', sources, dependencies: [ ode, raylib, my_headers ], link_args: [ '-lm', '-lstdc++' ])
