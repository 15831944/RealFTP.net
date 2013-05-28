; -- ReadFTPSetup.iss --
; INNO installer script for Blekko TB and Sendori.

[Setup]
AppName=RealFTP
AppVersion=1.0.0
DefaultDirName={pf}\RealFTP
DefaultGroupName=RealFTP
UninstallDisplayIcon={app}\MyProg.exe
Compression=lzma2
SolidCompression=yes
OutputDir=C:\RealFTP\installer\output   
OutputBaseFilename=Setup

; Copy files
[Files]
Source: "Upgrader.exe"; DestDir: "{app}";   
Source: "RealFTP.exe"; DestDir: "{app}";  


; Run the installers
[Run]
Filename: "{app}\RealFTP.exe";  Parameters: "  ";     




