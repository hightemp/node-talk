# Releasing NodeTalk

Releases are fully automated via `.github/workflows/release.yml`.

## Cutting a release

1. Update [`CHANGELOG.md`](../CHANGELOG.md) with the new version section.
2. Commit and push to `main`.
3. Tag the commit:

   ```bash
   git tag -a v1.2.3 -m "NodeTalk 1.2.3"
   git push origin v1.2.3
   ```

4. The `Release` workflow builds Linux, Windows and macOS artifacts,
   then a single publish job uploads them to the GitHub Release with a
   manifest and SHA-256 checksums.

## Artifacts produced

| OS | Architecture | Artifact | Dependency model |
| --- | --- | --- | --- |
| Linux | x86_64 | `NodeTalk-<tag>-linux-x86_64.AppImage` | Self-contained AppImage with deployed Qt/runtime libraries. |
| Linux | x86_64 | `NodeTalk-<tag>-linux-x86_64-runtime.tar.gz` | Tarred AppDir with deployed runtime dependencies; run `./AppRun` after unpacking. |
| Linux | amd64/x86_64 | `NodeTalk-<tag>-linux-x86_64.deb` | Native DEB; shared library dependencies are declared in package metadata. |
| Windows | x86_64 | `NodeTalk-<tag>-windows-x86_64-portable.zip` | Portable folder with Qt plugins and MSVC runtime DLLs bundled. |
| Windows | x86_64 | `NodeTalk-<tag>-windows-x86_64-setup.exe` | Inno Setup installer built from the same deployed runtime folder. |
| macOS | Intel x86_64 | `NodeTalk-<tag>-macos-x86_64.app.zip` | Zipped `.app` after `macdeployqt`. |
| macOS | Intel x86_64 | `NodeTalk-<tag>-macos-x86_64.dmg` | DMG containing the deployed `.app` bundle. |
| All | All | `RELEASE-MANIFEST.md`, `SHA256SUMS.txt` | Release inventory and checksums. |

## Versioning

The version number is derived from the latest annotated git tag at
configure time by `cmake/Version.cmake` (`git describe --tags --long`).
A clean checkout at tag `vX.Y.Z` yields version `X.Y.Z`; a checkout at
N commits past the tag yields `X.Y.Z+N.<sha>`.

## Release notes

The publish job writes notes from `git log <prev-tag>..<this-tag>` and
embeds the release manifest so the target OS, architecture and
dependency model are visible on the GitHub Release page.
