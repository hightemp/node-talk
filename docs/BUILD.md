# Building NodeTalk

## Prerequisites

* CMake **3.21+**
* Qt **6.4+** (Core, Gui, Widgets, Network, Sql, LinguistTools, Test)
* A C++17 compiler (GCC 11+, Clang 14+, MSVC 2019+)

## Common

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The executable is `build/NodeTalk` (Linux/Windows) or
`build/NodeTalk.app` (macOS).

To run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Linux

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev \
  qt6-tools-dev-tools libgl1-mesa-dev
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/qt6
cmake --build build --parallel
```

## Windows

Open *x64 Native Tools Command Prompt* and ensure Qt's `bin\` is on
`PATH` (or pass `-DCMAKE_PREFIX_PATH=C:\Qt\6.7.2\msvc2019_64`).

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## macOS

```bash
brew install qt cmake
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open build/NodeTalk.app
```

## Build options

| Option | Default | Effect |
| --- | --- | --- |
| `NODETALK_BUILD_TESTS` | `ON` | Build the Qt Test suite |
| `CMAKE_BUILD_TYPE`     | `Release` | Standard CMake setting |

## Translations

`.ts` files live under `resources/i18n/`. They are compiled to `.qm`
automatically by `qt_add_translations` during the build. To refresh
translatable strings from sources, run:

```bash
cmake --build build --target update_translations
```
