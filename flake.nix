
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    lf.url = "github:dev-dwarf/lf?ref=main";
  };

  outputs = { self, nixpkgs, lf, ... }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      inherit system;
      overlays = [lf.overlay];
    };
  in rec {
    packages.${system} = pkgs // rec {
      site = pkgs.mkSTU rec {
        name = "site";
        src = ./src;
      };

      html = pkgs.stdenv.mkDerivation rec {
        name = "html";
        src = ./.; 
        allowSubstitutes = false;
        buildInputs = [ site ];
        buildPhase = ''
          mkdir -p $out/docs/writing
          cp -r docs/ $out/
          cp -r pages/ $out/
          cp src/*ml $out/
          cd $out && site
        '';
        installPhase = ''
          mkdir -p $out
          cp -r docs/* $out
        '';
      };
    };

  };
}
