# SnipNexs 0.8.1 winget manifest (template)

Submit to <https://github.com/microsoft/winget-pkgs> via PR into
`manifests/y/YDLuo/SnipNexs/0.8.1/`. Winget expects three separate
multi-file manifests; do not merge them into a singleton.

Replace `InstallerSha256` with the SHA-256 of `SnipNexs-0.8.1-win64.zip`
printed by `scripts/package-release.ps1` (GitHub also shows the digest next to
the release asset). Validate with `winget validate <manifest-dir>` before
opening the PR.

This template is manual: the release script does not generate or submit winget
manifests.

## YDLuo.SnipNexs.yaml

```yaml
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.1
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.6.0
```

## YDLuo.SnipNexs.installer.yaml

```yaml
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.1
MinimumOSVersion: 10.0.19041.0
Installers:
  - Architecture: x64
    InstallerType: zip
    NestedInstallerType: portable
    NestedInstallerFiles:
      - RelativeFilePath: bin/SnipNexs.exe
        PortableCommandAlias: snipnexs
    InstallerUrl: https://github.com/YDLuo-1/SnipNexs/releases/download/v0.8.1/SnipNexs-0.8.1-win64.zip
    InstallerSha256: c0f650afc0d6afb594fbc3ff71cb037a38b5a2af27681ac3bedb4db559ce2b49
ManifestType: installer
ManifestVersion: 1.6.0
```

## YDLuo.SnipNexs.locale.en-US.yaml

```yaml
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.1
PackageLocale: en-US
Publisher: YDLuo
PublisherUrl: https://github.com/YDLuo-1/SnipNexs
PublisherSupportUrl: https://github.com/YDLuo-1/SnipNexs/issues
PackageName: SnipNexs
License: GPL-3.0-or-later
LicenseUrl: https://github.com/YDLuo-1/SnipNexs/blob/main/LICENSE
ShortDescription: Lightweight screenshot, OCR and offline translation toolkit for Windows
Description: >
  Region capture with hierarchical window targeting, annotation, a local color
  picker, pinned images with zoom, Windows OCR, fully offline translation
  (OPUS-MT by default with an optional Hy-MT2-1.8B high-quality engine) and
  GPU-backed region recording. No telemetry; everything stays on the machine.
Tags:
  - screenshot
  - screen-capture
  - ocr
  - translation
  - recording
ManifestType: defaultLocale
ManifestVersion: 1.6.0
```
