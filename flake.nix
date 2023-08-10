{
  description = "game gaming";

  inputs.nixpkgs.url = "nixpkgs/nixos-unstable";

  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {inherit system;};
  in {
    devShell.${system} =
      pkgs.mkShell.override
      {
        # use gcc for libc headers in intellisense
        stdenv = pkgs.gccStdenv;
      }
      {
        packages =
          (with pkgs; [
            gdb
            valgrind
            pkg-config
            libGL
            libGLU
            zig_0_11
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
