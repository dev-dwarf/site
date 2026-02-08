
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in rec {

    mkSTU = let 
      mk = {name, src
      , main ? name #default main src file is $name.cpp or $name.c
      , buildInputs ? []
      , flags ? ""
      , debug ? false
      , cpp ? false
      , executable ? true
      , installPhase ? ''
        runHook preInstall;
        install -Dt $out/bin ${name};
        runHook postInstall;
      ''
      , ...}@args:
        pkgs.stdenv.mkDerivation (args // {
          inherit name src installPhase buildInputs;
          doCheck = !debug;
          dontStrip = debug;
          allowSubstitutes = false;
          buildPhase = 
          ''${if cpp then "$CXX" else "$CC"} \
            ${if executable then "-o ${name}" else "-o {name}.o -c"} \
            ${src}/${main}.${if cpp then "cpp" else "c"} \
            ${flags} ${if debug then "-g -O0" else "-O2"}
          '';
        });
    in args: (mk args) // { debug = mk (args // { debug = true; }); };

    packages.${system} = pkgs // rec {
      lf = pkgs.stdenv.mkDerivation rec {
        name = "lf";
        src = pkgs.fetchgit {
          url = "https://github.com/dev-dwarf/lf.git";
          hash = "sha256-Y+uriN4SmgdYF7w3jbuyLlZJj1JhHV4OID03KE4aHB4=";
        };
        dontBuild = true;
        allowSubstitutes = false;
        installPhase = '' mkdir -p $out/include && cp -r $src/* $out/include/'';
      };

      site = mkSTU rec {
        buildInputs = [ lf ];
        name = "site";
        src = ./src;
      };

    };

  };
}
