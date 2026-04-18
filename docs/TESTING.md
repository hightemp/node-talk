# Testing strategy

## Layers

| Test | What it covers | Type |
| --- | --- | --- |
| `tst_protocol`     | `encodeJson` / `encodeBinary` round-trip; `FrameReader` chunked / multi-frame / oversize | Unit |
| `tst_database`     | Schema migration; peer / message / transfer repositories | Unit (SQLite on disk) |
| `tst_identity`     | Identity persists across `Database` reload | Unit |
| `tst_filetransfer` | TCP frame-level chunk transfer round-trip | Integration |
| `tst_smoke`        | `app::Application::initialize()` + `shutdown()` under offscreen QPA | Smoke |

## Conventions

* Each test is a separate Qt Test executable so failures are isolated
  and `ctest -j` parallelizes them.
* Tests run with `QT_QPA_PLATFORM=offscreen` (set per-test in
  `tests/CMakeLists.txt`).
* `Database` instances use unique connection names so multiple tests
  may run concurrently without clashing on the global Qt SQL registry.
* No test reaches the public network. UDP discovery and multicast are
  exercised only in interactive manual smoke tests.

## Running

```bash
cmake -S . -B build -DNODETALK_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Add `-j8` to parallelize. To re-run a single test verbosely:

```bash
ctest --test-dir build -R tst_protocol -V
```

## Adding a test

1. Create `tests/tst_<name>.cpp` with a `QTEST_GUILESS_MAIN(...)` (or
   `QTEST_MAIN(...)` for tests that need `QApplication`).
2. Append `tst_<name>` to `NODETALK_TESTS` in `tests/CMakeLists.txt`.
3. Run `cmake --build build` — the new target is picked up automatically.
