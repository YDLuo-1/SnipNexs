; SnipNexs installer script (Inno Setup 6).
; Prerequisite: run scripts/package-release.ps1 first; this script packages
; the staged layout in dist/SnipNexs-0.8.0-win64 (bin/, licenses/, plugins/,
; LICENSE, THIRD_PARTY_NOTICES.md).
;
; Build: iscc scripts\installer.iss
; Output: dist/SnipNexs-0.8.0-setup.exe

#define AppVersion "0.8.0"
#define AppName "SnipNexs"
#define AppPublisher "SnipNexs contributors"
#define AppExe "bin\SnipNexs.exe"
#define StageDir "..\dist\SnipNexs-0.8.0-win64"

[Setup]
AppId={{8E5F1D62-4C3A-4B8E-9A21-6F0D2C5B9A01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=SnipNexs-{#AppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#AppExe}

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; capture history and downloaded translation models are user data; keep them
Type: filesandordirs; Name: "{app}\bin\history"
Type: filesandordirs; Name: "{app}\bin\models"
