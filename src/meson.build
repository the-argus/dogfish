project('lungfish', 'c', default_options: 'default_library=static')

add_global_arguments('-Wall', language : 'c')
add_global_arguments('-Werror', language : 'c')
add_global_arguments('-pedantic', language : 'c')
add_global_arguments('-O2', language : 'c')

raylib = dependency('raylib')

ode = dependency('ode')

source_file_names = [
  'main.c',
  'input.c',
  'camera_manager.c',
  'fps_camera.c',
  'physics.c'
]

sources = []

foreach file : source_file_names
  sources += files(file)
endforeach

my_headers = declare_dependency(include_directories : include_directories('include', '.'))

executable('lungfish', sources, dependencies: [ ode, raylib, my_headers ], link_args: [ '-lm', '-lstdc++' ])
