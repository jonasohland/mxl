# SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
# SPDX-License-Identifier: Apache-2.0

# The GStreamer plugins in rust/: gstmxl (mxlsrc, mxlsink) from gst-mxl-rs and
# gstavsynctest from gst-avsynctest-rs. They are installed to
# lib/gstreamer-1.0.
{
  lib,
  makeRustPlatform,
  rustToolchain,
  pkg-config,
  patchelf,
  gst_all_1,
  mxl,
  src,
  version,
}:

let
  rustPlatform = makeRustPlatform {
    cargo = rustToolchain;
    rustc = rustToolchain;
  };
in
rustPlatform.buildRustPackage {
  pname = "gst-plugins-mxl";
  inherit version src;

  cargoRoot = "rust";
  buildAndTestSubdir = "rust";
  cargoLock.lockFile = ../rust/Cargo.lock;

  nativeBuildInputs = [
    pkg-config
    patchelf
    rustPlatform.bindgenHook
  ];

  buildInputs = [
    mxl
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ];

  # The tests build pipelines with elements such as videotestsrc and appsink.
  checkInputs = with gst_all_1; [
    gst-plugins-good
    gst-plugins-bad
  ];

  # Link mxl-sys against the mxl package instead of building lib/ with the
  # vcpkg based CMake preset, which is not possible in the Nix sandbox.
  env.MXL_PREBUILT_DIR = "${mxl}";

  cargoBuildFlags = [
    "--package=gst-mxl-rs"
    "--package=gst-avsynctest-rs"
  ];
  cargoTestFlags = [ "--workspace" ];

  # Same as CI, which runs the tests with --test-threads=1.
  dontUseCargoParallelTests = true;

  postInstall = ''
    mkdir -p "$out/lib/gstreamer-1.0"
    mv "$out"/lib/libgst*.so "$out/lib/gstreamer-1.0/"
  '';

  # The plugins load libmxl.so with dlopen at runtime and do not link it, so
  # the fixup phase would drop it from the RUNPATH. Add it back afterwards.
  postFixup = ''
    for plugin in "$out"/lib/gstreamer-1.0/*.so; do
      patchelf --add-rpath "${mxl}/lib" "$plugin"
    done
  '';

  meta = {
    description = "GStreamer plugins for the Media eXchange Layer";
    homepage = "https://github.com/dmf-mxl/mxl";
    license = lib.licenses.asl20;
    platforms = lib.platforms.linux;
  };
}
