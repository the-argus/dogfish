project('lungfish', 'c', default_options: 'default_library=static')

raylib = dependency('raylib')

source_file_names = [
  'main.c'
]

sources = []

foreach file : source_file_names
  sources += files(file)
endforeach

my_headers = declare_dependency(include_directories : include_directories('include'))

executable('lungfish', sources, dependencies: [ raylib, my_headers ])
