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

4. The `Release` workflow runs three matrix jobs (Linux / Windows /
   macOS) and uploads the artifacts to the auto-created GitHub Release.

## Artifacts produced

| OS | Artifact |
| --- | --- |
| Linux   | `NodeTalk-<v>-x86_64.AppImage` |
| Linux   | `NodeTalk-<v>-Linux.tar.gz`    |
| Linux   | `nodetalk_<v>_amd64.deb`       |
| Windows | `NodeTalk-<v>-win64.zip`       |
| Windows | `NodeTalk-<v>-Setup.exe` (Inno Setup) |
| macOS   | `NodeTalk-<v>-macos.zip` (zipped `.app`) |
| macOS   | `NodeTalk-<v>.dmg`             |

## Versioning

The version number is derived from the latest annotated git tag at
configure time by `cmake/Version.cmake` (`git describe --tags --long`).
A clean checkout at tag `vX.Y.Z` yields version `X.Y.Z`; a checkout at
N commits past the tag yields `X.Y.Z+N.<sha>`.

## Release notes

`softprops/action-gh-release` auto-generates notes from PR titles and
commit messages. Polish them on the GitHub release page if needed.
