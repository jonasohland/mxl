# SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
# SPDX-License-Identifier: Apache-2.0

{
  description = "Media eXchange Layer (MXL) SDK, tools and GStreamer plugins";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      rust-overlay,
    }:
    let
      inherit (nixpkgs) lib;
      fs = lib.fileset;

      # The Rust plugins load libmxl.so by name and libfabric is Linux only, so
      # only Linux systems are listed.
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems =
        f:
        lib.genAttrs systems (
          system:
          f (
            import nixpkgs {
              inherit system;
              overlays = [
                rust-overlay.overlays.default
                self.overlays.default
              ];
            }
          )
        );

      mxlVersion = builtins.head (
        builtins.match ".*set\\(mxl_VERSION ([0-9.]+)\\).*" (builtins.readFile ./CMakeLists.txt)
      );
      # Same format as git rev-parse --short=12 HEAD in CMakeLists.txt.
      shortRev = rev: builtins.substring 0 12 rev;
      selfGitHash =
        if self ? rev then
          shortRev self.rev
        else if self ? dirtyRev then
          "${shortRev self.dirtyRev}-dirty"
        else
          null;

      # Upstream release tags, built from the tagged source with the recipe in
      # nix/mxl.nix. Each entry has the tagged commit and the hash of its
      # source. Patches in nix/patches/<version>/ are applied to that release.
      # The release-tags workflow adds new tags.
      releases = lib.importJSON ./nix/releases.json;
      releasePatches =
        version:
        let
          dir = ./nix/patches + "/${version}";
        in
        lib.optionals (builtins.pathExists dir) (
          map (name: dir + "/${name}") (
            lib.sort lib.lessThan (
              lib.attrNames (lib.filterAttrs (_: type: type == "regular") (builtins.readDir dir))
            )
          )
        );
      # mxl_1_1_0 for each release, and mxl_1_1 for the newest release of
      # each major.minor line.
      releaseAttr = version: "mxl_${lib.replaceStrings [ "." ] [ "_" ] version}";
      releaseLines = lib.mapAttrs' (
        line: versions:
        lib.nameValuePair (releaseAttr line) (releaseAttr (lib.last (lib.sort lib.versionOlder versions)))
      ) (lib.groupBy lib.versions.majorMinor (lib.attrNames releases));
      releaseAttrs = map releaseAttr (lib.attrNames releases);

      rustVersion = (builtins.fromTOML (builtins.readFile ./rust/Cargo.toml)).workspace.package.version;

      cmakeSrc = fs.toSource {
        root = ./.;
        fileset = fs.unions [
          ./CMakeLists.txt
          ./cmake
          ./lib
          ./tools
          ./utils
        ];
      };

      # mxl-sys reads the C headers from lib/ and the mxl tests read flow
      # definitions from lib/tests/data.
      rustSrc = fs.toSource {
        root = ./.;
        fileset = fs.difference (fs.unions [
          ./lib
          ./rust
        ]) (fs.maybeMissing ./rust/target);
      };

      # Same version as .devcontainer/scripts/common/rust/install-rust.sh.
      rustToolchainFor = pkgs: pkgs.rust-bin.stable."1.90.0".default;

      # Tools that tools/CMakeLists.txt builds. The fabrics tools are built
      # because the mxl package enables MXL_ENABLE_FABRICS_OFI on Linux.
      tools = [
        "mxl-info"
        "mxl-data-probe"
        "mxl-video-probe"
        "mxl-gst-testsrc"
        "mxl-gst-sink"
        "mxl-gst-looping-filesrc"
        "mxl-fabrics-demo"
        "mxl-fabrics-info"
      ];
    in
    {
      overlays.default =
        final: prev:
        {
          picojson = final.callPackage ./nix/picojson.nix { };
          pcapplusplus = final.callPackage ./nix/pcapplusplus.nix { };

          mxl = final.callPackage ./nix/mxl.nix {
            src = cmakeSrc;
            version = mxlVersion;
            gitHash = selfGitHash;
          };

          gst-plugins-mxl = final.callPackage ./nix/gst-plugins.nix {
            src = rustSrc;
            version = rustVersion;
            rustToolchain = rustToolchainFor final;
          };

          # gst-launch-1.0 and gst-inspect-1.0 with the MXL plugins and the
          # common upstream plugins on the plugin path.
          gst-mxl-env = final.symlinkJoin {
            name = "gst-mxl-env-${rustVersion}";
            paths = [ final.gst_all_1.gstreamer.bin ];
            nativeBuildInputs = [ final.makeWrapper ];
            postBuild = ''
              for tool in "$out"/bin/gst-*; do
                wrapProgram "$tool" \
                  --prefix GST_PLUGIN_SYSTEM_PATH_1_0 : "${
                    lib.makeSearchPathOutput "lib" "lib/gstreamer-1.0" (
                      [
                        final.gst-plugins-mxl
                        final.mxl
                      ]
                      ++ final.mxl.gstRuntimePlugins
                    )
                  }"
              done
            '';
            meta.mainProgram = "gst-launch-1.0";
          };
        }
        // lib.mapAttrs' (
          version: release:
          lib.nameValuePair (releaseAttr version) (
            final.mxl.override {
              inherit version;
              src = final.fetchFromGitHub {
                owner = "dmf-mxl";
                repo = "mxl";
                inherit (release) rev hash;
              };
              patches = releasePatches version;
              gitHash = shortRev release.rev;
            }
          )
        ) releases
        // lib.mapAttrs (_: target: final.${target}) releaseLines;

      packages = forAllSystems (
        pkgs:
        {
          inherit (pkgs)
            mxl
            gst-plugins-mxl
            gst-mxl-env
            picojson
            ;
          default = pkgs.mxl;
        }
        // lib.getAttrs (releaseAttrs ++ lib.attrNames releaseLines) pkgs
      );

      # nix run .#<tool> for each tool, plus gst-launch and gst-inspect with
      # the MXL plugins loaded.
      apps = forAllSystems (
        pkgs:
        let
          mkApp = drv: name: {
            type = "app";
            program = "${drv}/bin/${name}";
            meta.description = "${name} from ${drv.name}";
          };
        in
        lib.genAttrs tools (mkApp pkgs.mxl)
        // {
          default = mkApp pkgs.mxl "mxl-info";
          gst-launch = mkApp pkgs.gst-mxl-env "gst-launch-1.0";
          gst-inspect = mkApp pkgs.gst-mxl-env "gst-inspect-1.0";
        }
      );

      devShells = forAllSystems (
        pkgs:
        let
          llvm = pkgs.llvmPackages_19;
          rustToolchain = (rustToolchainFor pkgs).override {
            extensions = [
              "rust-src"
              "rust-analyzer"
              "clippy"
              "rustfmt"
            ];
          };
          # The CMake presets load $VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake.
          # In this shell the dependencies come from Nix, so the presets get a
          # toolchain file that does not install anything. This also lets the
          # mxl-sys build script build lib/ with its preset.
          #
          # The file turns off C++20 module scanning. MXL has no modules, and
          # clang-scan-deps does not see the include paths that the Nix
          # compiler wrapper adds, so the Clang presets fail without this.
          vcpkgStub = pkgs.writeTextDir "scripts/buildsystems/vcpkg.cmake" ''
            # Dependencies come from the Nix dev shell.
            set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
          '';
          # The Clang presets link with -fuse-ld=lld. Without this, clang uses
          # the host's ld.lld, which does not add the run paths of Nix
          # libraries, and the binaries fail to load them. Only ld.lld is
          # linked, so the GCC presets keep GNU ld.
          wrappedLld = pkgs.runCommand "ld.lld-wrapped" { } ''
            mkdir -p "$out/bin"
            ln -s ${llvm.bintools}/bin/ld.lld "$out/bin/ld.lld"
          '';
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ pkgs.mxl ];

            packages = [
              rustToolchain
              pkgs.rustPlatform.bindgenHook
              pkgs.catch2_3
              pkgs.cargo-audit
              pkgs.cargo-deny
              pkgs.cargo-machete
              pkgs.cargo-nextest
              pkgs.cargo-outdated
              llvm.clang
              llvm.clang-tools
              wrappedLld
              pkgs.ccache
              pkgs.doxygen
              pkgs.gdb
              pkgs.reuse
              pkgs.gst_all_1.gstreamer.bin
            ]
            ++ pkgs.mxl.gstRuntimePlugins;

            VCPKG_ROOT = "${vcpkgStub}";
          };
        }
      );

      checks = forAllSystems (
        pkgs:
        {
          inherit (pkgs) mxl gst-plugins-mxl gst-mxl-env;
        }
        // lib.getAttrs releaseAttrs pkgs
      );

      formatter = forAllSystems (pkgs: pkgs.nixfmt);
    };
}
