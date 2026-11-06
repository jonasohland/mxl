# SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
# SPDX-License-Identifier: Apache-2.0

# picojson is not in nixpkgs. The header is installed as
# include/picojson/picojson.h, which is the layout vcpkg uses and the one
# cmake/modules/Findpicojson.cmake looks for.
{
  lib,
  stdenvNoCC,
  fetchFromGitHub,
}:

stdenvNoCC.mkDerivation (finalAttrs: {
  pname = "picojson";
  version = "1.3.0";

  src = fetchFromGitHub {
    owner = "kazuho";
    repo = "picojson";
    tag = "v${finalAttrs.version}";
    hash = "sha256-wMYfVuDEBCrU0cl32suCiM9yaayvy1zRkrqVMMLSntY=";
  };

  dontConfigure = true;
  dontBuild = true;

  installPhase = ''
    runHook preInstall
    install -Dm644 picojson.h "$out/include/picojson/picojson.h"
    runHook postInstall
  '';

  meta = {
    description = "Header-only JSON parser and serializer for C++";
    homepage = "https://github.com/kazuho/picojson";
    license = lib.licenses.bsd2;
    platforms = lib.platforms.all;
  };
})
