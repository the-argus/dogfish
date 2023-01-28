project('lungfish', 'c', default_options: 'default_library=static')

raylib = dependency('raylib')

source_file_names = [
  'main.c'
]

sources = []

foreach file : source_file_names
  sources += files(file)
endforeach

executable('lungfish', sources, dependencies: raylib)
