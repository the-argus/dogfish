{
  stdenv,
  ninja,
  pkg-config,
  raylib,
  ode,
  meson,
  ...
}:
stdenv.mkDerivation {
  name = "dogfish";
  src = ./src;
  nativeBuildInputs = [
    pkg-config
    raylib
    ode
    meson
    ninja
  ];
  configurePhase = ''
    meson setup builddir
  '';
  buildPhase = ''
    meson compile -C builddir
  '';
  installPhase = ''
    mkdir -p $out/bin
    mv builddir/dogfish $out/bin/dogfish
  '';
}
