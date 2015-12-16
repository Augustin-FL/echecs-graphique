; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=Echec Graphique
AppVersion=1.5
DefaultDirName={pf}\Echecs Graphique
DefaultGroupName=Echecs Graphique
UninstallDisplayIcon={app}\Echecs Graphique.exe
Compression=lzma2
SolidCompression=yes
OutputDir=.

[Files]
Source: "libfreetype-6.dll"; DestDir: "{app}"
Source: "libgcc_s_dw2-1.dll"; DestDir: "{app}"
Source: "main.exe"; DestDir: "{app}"
Source: "README.md"; DestDir: "{app}"
Source: "SDL.dll"; DestDir: "{app}"
Source: "SDL_ttf.dll"; DestDir: "{app}"
Source: "zlib1.dll"; DestDir: "{app}"
Source: "verdana.ttf"; DestDir: "{app}"
Source: "pieces.bmp"; DestDir: "{app}"


[Icons]
Name: "{group}\Echecs Graphique"; Filename: "{app}\main.exe"
