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
    pkgs = import nixpkgs {localSystem = {inherit system;};};
    regularPkgs = pkgs;
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
          cmake = pkgs.cmake;
          pkg-config = pkgs.pkg-config;
          raylib-games = pkgs.emptyDirectory;
          gfortran = pkgs.gfortran;
          libpulseaudio = pkgs.libpulseaudio;
          mesa = pkgs.mesa;
          gettext = pkgs.gettext;
        })
        (_: _: {
          ninja = pkgs.ninja.override {python3 = pkgs.python3Minimal;};
        })
        (_: _: {
          meson = pkgs.meson.override {python3 = pkgs.python3Minimal;};
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
        buildPhase = ''
          meson setup builddir
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
      lungfish = pkgs.callPackage lungfish {};
      default = lungfish;
    };
    devShell.${system} =
      pkgs.mkShell.override
      {
        stdenv = pkgs.clangStdenv;
      }
      {
        packages = with pkgs; [
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
