; NodeTalk Inno Setup script.
; Build with: ISCC.exe /DAppVersion=1.2.3 /DSourceDir=path\to\windeployqt-output nodetalk.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\build\install"
#endif

[Setup]
AppId={{CC4D6F62-1B4A-4D3F-9A0B-9E8F9F0B3E10}
AppName=NodeTalk
AppVersion={#AppVersion}
AppPublisher=NodeTalk Project
AppPublisherURL=https://github.com/your-org/node-talk
DefaultDirName={autopf}\NodeTalk
DefaultGroupName=NodeTalk
DisableProgramGroupPage=yes
OutputBaseFilename=NodeTalk-{#AppVersion}-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\NodeTalk.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\NodeTalk"; Filename: "{app}\NodeTalk.exe"
Name: "{commondesktop}\NodeTalk"; Filename: "{app}\NodeTalk.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\NodeTalk.exe"; Description: "Launch NodeTalk"; Flags: nowait postinstall skipifsilent
