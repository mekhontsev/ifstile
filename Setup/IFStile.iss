
#define BIN_PATH      "..\Bin\IFStile.exe"
#define BIN_PATH_ARM  "..\Bin\IFStileARM.exe"

#define APPNAME      GetStringFileInfo(BIN_PATH, PRODUCT_NAME)
#define APPVERSION   GetStringFileInfo(BIN_PATH, PRODUCT_VERSION)
#define APPPUBLISHER GetStringFileInfo(BIN_PATH, COMPANY_NAME)
#define APPCOPYRIGHT GetStringFileInfo(BIN_PATH, LEGAL_COPYRIGHT)
#define VERSIONINFO  GetFileVersion(BIN_PATH)

#define APPID  APPNAME
#define APPURL "https://ifstile.com/"

#define MUTEX "E93A0F8F-5F94-41F4-BCFB-BC4952844D82"

[Setup]
AppName={#APPNAME}
AppId={#APPID}

AppMutex={#MUTEX}
SetupMutex={#MUTEX}

WizardStyle=modern

AppVersion={#APPVERSION}
AppPublisher={#APPPUBLISHER}
AppCopyright={#APPCOPYRIGHT}

DefaultDirName={pf}\{#APPNAME}
DefaultGroupName={#APPNAME}
VersionInfoVersion={#VERSIONINFO}

AppPublisherURL={#APPURL}
AppSupportURL={#APPURL}
AppUpdatesURL={#APPURL}
LicenseFile=License.rtf
PrivilegesRequired=admin
AppComments=Release

;WizardImageFile=compiler:WizModernImage-IS.bmp
;WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp
;InfoBeforeFile=info_before.txt
;InfoAfterFile=info_after.txt
OutputDir="."

OutputBaseFilename=IFStileSetup
ArchitecturesAllowed=x64compatible 
ArchitecturesInstallIn64BitMode=x64compatible

UninstallDisplayIcon={app}\{#APPNAME}.exe

;SetupIconFile=Files\setup.ico
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
DisableProgramGroupPage=yes

MinVersion=6.1.7601
     
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"


[Files]
Source: {#BIN_PATH}; DestDir: "{app}"; Flags: ignoreversion;  DestName: "{#APPNAME}.exe"; Check: IsX64OS
Source: {#BIN_PATH_ARM}; DestDir: "{app}"; Flags: ignoreversion;  DestName: "{#APPNAME}.exe"; Check: IsArm64
        
[Run]
Filename: "{app}\{#APPNAME}.exe"; Description: "Launch application"; Flags: postinstall nowait skipifsilent unchecked

[Dirs]
Name: "{userdocs}\{#APPNAME}"; Flags: uninsneveruninstall

[Icons]
Name: "{group}\{#APPNAME}"; Filename: "{app}\{#APPNAME}.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\{#APPNAME}"; Filename: "{app}\{#APPNAME}.exe";


[Registry]
Root: HKCR; Subkey: ".aifs"; ValueType: string; ValueName: ""; ValueData: "{#APPNAME}.Document"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "{#APPNAME}.Document"; ValueType: string; ValueName: ""; ValueData: "{#APPNAME}.Document"; Flags: uninsdeletekey
Root: HKCR; Subkey: "{#APPNAME}.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#APPNAME}.exe,0";  Flags: uninsdeletevalue
Root: HKCR; Subkey: "{#APPNAME}.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#APPNAME}.exe"" ""%1"""; Flags: uninsdeletevalue
[Code]



function InitializeSetup(): Boolean;
var
	UninstString: String;
	Response: Integer;
	ResultCode: Integer;
begin

  Result:=TRUE;
 
end;

