; SnipNexs installer script (Inno Setup 6).
; Prerequisite: run scripts/package-release.ps1 first; this script packages
; the staged layout in dist/SnipNexs-0.8.5-win64 (bin/, licenses/, plugins/,
; LICENSE, THIRD_PARTY_NOTICES.md).
;
; Build: iscc scripts\installer.iss
; Output: dist/SnipNexs-0.8.5-setup.exe

#define AppVersion "0.8.5"
#define AppName "SnipNexs"
#define AppPublisher "SnipNexs contributors"
#define AppExe "bin\SnipNexs.exe"
#define StageDir "..\dist\SnipNexs-0.8.5-win64"

[Setup]
AppId={{8E5F1D62-4C3A-4B8E-9A21-6F0D2C5B9A01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
SetupIconFile=..\resources\app.ico
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=SnipNexs-{#AppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
PrivilegesRequired=admin
CloseApplications=yes
UninstallDisplayIcon={app}\{#AppExe}

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Deliberate allowlist: a used portable deployment may contain private captures/settings.
Source: "{#StageDir}\bin\SnipNexs.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#StageDir}\bin\*.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#StageDir}\bin\qt.conf"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#StageDir}\plugins\*.dll"; DestDir: "{app}\plugins"; Flags: recursesubdirs ignoreversion
Source: "{#StageDir}\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\docs\local-translation-decision.md"; DestDir: "{app}\docs"; Flags: ignoreversion
; Models are optional, downloaded by the application after installation.
; Portable offline models and all runtime data stay outside this installer.

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

; bin\history and bin\models are user data. They are deliberately not listed
; in [UninstallDelete]; Inno Setup leaves files it did not install in place.
