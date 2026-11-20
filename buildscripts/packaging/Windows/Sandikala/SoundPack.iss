; Compile on Windows with Inno Setup 6:
; ISCC /DSoundfontDir="C:\path\to\soundfont" SoundPack.iss
#ifndef SoundfontDir
  #error SoundfontDir must point to the folder containing Gamelan_*.sf2
#endif
[Setup]
AppId={{74513AA9-BBD2-440F-B8AA-DF527CF7CC8B}
AppName=Sandikala Gamelan Sound Pack
AppVersion=1.0
DefaultDirName={autopf}\Sandikala-MuseScore
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=Sandikala-Gamelan-Sound-Pack
Compression=lzma2
SolidCompression=yes
DiskSpanning=yes
DiskSliceSize=1500000000
[Files]
Source: "{#SoundfontDir}\Gamelan_*.sf2"; DestDir: "{app}\sound"; Flags: ignoreversion
Source: "{#SoundfontDir}\Gamelan_Open_Sample_Attribution.txt"; DestDir: "{app}\sound"; Flags: ignoreversion skipifsourcedoesntexist
