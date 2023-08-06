{
  description = "game gaming";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-23.05";
  };

  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      localSystem = {inherit system;};
      overlays = import ./build/nix/overlays.nix;
    };
  in {
    devShell.${system} =
      pkgs.mkShell.override
      {
        stdenv = pkgs.clangStdenv;
      }
      {
        packages =
          (with pkgs; [
            gdb
            valgrind
            pkg-config
            libGL
            libGLU
            zig
          ])
          ++ (with pkgs.xorg; [
            libX11
            libXrandr
            libXinerama
            libXcursor
            libXi
          ]);
      };
  };
}
