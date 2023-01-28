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
    pkgs = import nixpkgs {inherit system;};
  in {
    devShell.${system} =
      pkgs.mkShell.override {
        stdenv = pkgs.clangStdenv;
      } {
        packages = with pkgs; [
          pkg-config
          raylib
          meson
          bear
          clang-tools
        ];
      };
  };
}
