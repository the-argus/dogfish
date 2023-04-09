cd src
meson setup builddir
meson compile -C builddir
cd ..
start src\builddir\dogfish.exe
