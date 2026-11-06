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
      overlays.default = final: prev: {
        picojson = final.callPackage ./nix/picojson.nix { };
        pcapplusplus = final.callPackage ./nix/pcapplusplus.nix { };

        mxl = final.callPackage ./nix/mxl.nix {
          src = cmakeSrc;
          version = mxlVersion;
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
      };

      packages = forAllSystems (pkgs: {
        inherit (pkgs)
          mxl
          gst-plugins-mxl
          gst-mxl-env
          picojson
          ;
        default = pkgs.mxl;
      });

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

      checks = forAllSystems (pkgs: {
        inherit (pkgs) mxl gst-plugins-mxl gst-mxl-env;
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt);
    };
}
