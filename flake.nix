{
  description = "game gaming";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-22.11";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    system = "x86_64-linux";
    regularPkgs = import nixpkgs {localSystem = {inherit system;};};
    clangMuslPkgs = import nixpkgs {
      localSystem = {
        inherit system;
        libc = "musl";
        config = "x86_64-unknown-linux-musl";
      };
      config.replaceStdenv = {pkgs, ...}:
        pkgs.clangStdenv.override {
          cc = regularPkgs.buildPackages.clang;
        };
      # make lungfish's build system and some deps come from standard packages
      overlays = [
        (_: _: {
          cmake = regularPkgs.cmake;
          pkg-config = regularPkgs.pkg-config;
          raylib-games = regularPkgs.emptyDirectory;
          gfortran = regularPkgs.gfortran;
          libpulseaudio = regularPkgs.libpulseaudio;
          mesa = regularPkgs.mesa;
          gettext = regularPkgs.gettext;
        })
        (_: _: {
          ninja = regularPkgs.ninja.override {python3 = regularPkgs.python3Minimal;};
        })
        (_: _: {
          meson = regularPkgs.meson.override {python3 = regularPkgs.python3Minimal;};
        })
        (_: super: {
          raylib = super.raylib.override {sharedLib = false;};
        })
      ];
    };

    lungfish = {
      stdenv,
      ninja,
      pkg-config,
      raylib,
      meson,
      ...
    }:
      stdenv.mkDerivation {
        name = "lungfish";
        src = ./src;
        nativeBuildInputs = [
          pkg-config
          raylib
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
          mv builddir/lungfish $out/bin/lungfish
        '';
      };
  in {
    packages.${system} = {
      lungfishMusl = clangMuslPkgs.callPackage lungfish {};
      lungfish = regularPkgs.callPackage lungfish {stdenv = regularPkgs.clangStdenv;};
      default = self.packages.${system}.lungfish;
    };
    devShell.${system} =
      regularPkgs.mkShell.override
      {
        stdenv = regularPkgs.clangStdenv;
      }
      {
        packages = with regularPkgs; [
          (ninja.override {python3 = python3Minimal;})
          pkg-config
          raylib
          meson
          bear
          clang-tools
        ];
      };
  };
}
