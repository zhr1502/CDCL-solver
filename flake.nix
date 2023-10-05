{
  inputs = {
    nixpkgs.url = "flake:nixpkgs";
    # systems.url = "github:nix-systems/default";
  };

  outputs = {
    systems,
    nixpkgs,
    ...
  } @ inputs: let
    eachSystem = f:
      nixpkgs.lib.genAttrs (import systems) (
        system:
          f nixpkgs.legacyPackages.${system}
      );
  in {
    packages = eachSystem (pkgs: {
      hello = pkgs.hello;
    });

    devShells = eachSystem (pkgs: {
      default = pkgs.mkShell {
        buildInputs = with pkgs; [
          # Add development dependencies here
          gcc
          cmake
          gnumake
          clang-tools
          bear
          gdbgui
          gdb
        ];
      };
    });
  };
}
