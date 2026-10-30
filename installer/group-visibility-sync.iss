; SPDX-License-Identifier: GPL-2.0-or-later
#define PluginName "Group Visibility Sync for OBS Studio"
#define PluginVersion "0.1.0-beta.1"

[Setup]
AppId={{918D0C4A-CEDB-47F7-AF51-19C53C42AA3B}
AppName={#PluginName}
AppVersion={#PluginVersion}
AppPublisher=Kiddabob
AppPublisherURL=https://github.com/Kiddabob/obs-group-visibility-sync
AppSupportURL=https://github.com/Kiddabob/obs-group-visibility-sync/issues
AppUpdatesURL=https://github.com/Kiddabob/obs-group-visibility-sync
DefaultDirName={commonappdata}\obs-studio\plugins\group-visibility-sync
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..\downloads
OutputBaseFilename=GroupVisibilitySync-0.1.0-beta.1-Setup
InfoBeforeFile=installer-info.txt
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=no
RestartApplications=no
UninstallDisplayName=Group Visibility Sync for OBS Studio
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\local-build\Release\group-visibility-sync.dll"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
Name: "{app}\data"

[Code]
const
  GenericWrite = $40000000;
  OpenExisting = 3;
  FileAttributeNormal = $80;
  InvalidHandleValue = -1;

function CreateFileW(FileName: String; DesiredAccess, ShareMode: Cardinal;
  SecurityAttributes: Integer; CreationDisposition, FlagsAndAttributes: Cardinal;
  TemplateFile: Integer): Integer;
  external 'CreateFileW@kernel32.dll stdcall';
function CloseHandle(Handle: Integer): Boolean;
  external 'CloseHandle@kernel32.dll stdcall';

function PluginFileInUse: Boolean;
var
  PluginFile: String;
  Handle: Integer;
begin
  PluginFile := ExpandConstant('{app}\group-visibility-sync.dll');
  if not FileExists(PluginFile) then begin
    Result := False;
    exit;
  end;
  Handle := CreateFileW(PluginFile, GenericWrite, 0, 0, OpenExisting,
    FileAttributeNormal, 0);
  Result := Handle = InvalidHandleValue;
  if not Result then CloseHandle(Handle);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if PluginFileInUse then
    Result := 'The plugin file is in use or cannot be updated. Close the OBS ' +
      'instance using it, then retry the installation.';
end;

function InitializeUninstall: Boolean;
begin
  Result := not PluginFileInUse;
  if not Result then
    MsgBox('The plugin file is in use or cannot be removed. Close the OBS ' +
      'instance using it, then retry the uninstall.', mbError, MB_OK);
end;
