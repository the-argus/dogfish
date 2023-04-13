cd src
meson setup builddir
meson compile -C builddir || exit /b
cd ..
start src\builddir\dogfish.exe
