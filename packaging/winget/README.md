# SnipNexs 0.8.0 winget manifest (template)

Submit to https://github.com/microsoft/winget-pkgs via PR:
`manifests/y/YDLuo/SnipNexs/0.8.0/` with the three files below
(`YDLuo.SnipNexs.yaml`, `.installer.yaml`, `.locale.en-US.yaml`).
Fill `<SHA256>` with the SHA-256 of `SnipNexs-0.8.0-win64.zip` printed by
`scripts/package-release.ps1` (also shown on the GitHub Release).

---

# YDLuo.SnipNexs.yaml
YamlManifestType: singleton
ManifestVersion: 1.6.0
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.0
PackageName: SnipNexs
Publisher: YDLuo
License: GPL-3.0-or-later
LicenseUrl: https://github.com/YDLuo-1/SnipNexs/blob/main/LICENSE
ShortDescription: Lightweight screenshot, OCR and offline translation toolkit for Windows
PackageUrl: https://github.com/YDLuo-1/SnipNexs
Installers: []   # see .installer.yaml
ManifestType: singleton
ManifestVersion: 1.6.0

---

# YDLuo.SnipNexs.installer.yaml
ManifestVersion: 1.6.0
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.0
Installers:
  - Architecture: x64
    InstallerType: zip
    NestedInstallerType: portable
    NestedInstallerFiles:
      - RelativeFilePath: bin/SnipNexs.exe
        PortableCommandAlias: snipnexs
    InstallerUrl: https://github.com/YDLuo-1/SnipNexs/releases/download/v0.8.0/SnipNexs-0.8.0-win64.zip
    InstallerSha256: <SHA256>
ManifestType: installer
ManifestVersion: 1.6.0

---

# YDLuo.SnipNexs.locale.en-US.yaml
ManifestVersion: 1.6.0
PackageIdentifier: YDLuo.SnipNexs
PackageVersion: 0.8.0
PackageLocale: en-US
Publisher: YDLuo
PackageName: SnipNexs
License: GPL-3.0-or-later
ShortDescription: Lightweight screenshot, OCR and offline translation toolkit for Windows
Description: >
  Region capture with hierarchical window targeting, annotation, a local color
  picker, pinned images with zoom, Windows OCR, fully offline translation
  (OPUS-MT by default with an optional Hy-MT2-1.8B high-quality engine) and
  GPU-backed region recording. No telemetry; everything stays on the machine.
ManifestType: defaultLocale
ManifestVersion: 1.6.0
