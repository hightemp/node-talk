# Packaging

The repo ships ready-to-use packaging configuration for all three
target operating systems. CI consumes them automatically on tag push;
they can also be invoked manually.

## Linux

### Native tar.gz / .deb (CPack)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && cpack -G "TGZ;DEB"
```

Outputs a CPack install-tree `.tar.gz` and a native `.deb` in `build/`.
The release workflow uploads the `.deb`; the uploaded Linux runtime
`.tar.gz` is built from the linuxdeploy AppDir instead, so it contains
the deployed Qt/runtime dependencies.

### AppImage

Requires `linuxdeploy` and `linuxdeploy-plugin-qt` on `PATH`.

```bash
VERSION=1.0.0 bash packaging/linux/AppImage.sh
```

Produces `NodeTalk-<version>-x86_64.AppImage` in the repo root and a
dependency-bundled AppDir under `build/AppDir`.

## Windows

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
cmake --install build --prefix build\install --config Release
windeployqt --release --no-translations --compiler-runtime build\install\bin\NodeTalk.exe
```

The deployed folder contains the executable, Qt DLLs/plugins and the
compiler runtime. Then either:

* Zip `build\install\bin\` for a portable distribution, or
* Build the installer via [Inno Setup](https://jrsoftware.org/isinfo.php):

  ```cmd
  ISCC /DAppVersion=1.0.0 /DSourceDir=build\install\bin packaging\windows\nodetalk.iss
  ```

## macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
macdeployqt build/NodeTalk.app -dmg
```

Yields `build/NodeTalk.dmg` and a self-contained `.app` bundle with Qt
frameworks/plugins copied by `macdeployqt` (zip it with `ditto` for a
notarization-friendly archive).

For App Store / notarization steps see Apple's documentation; this
project does not embed signing identities.
