powershell rm .\X-Robot.zip
powershell sleep 1
echo "Making Folders..."
mkdir ReleasePack
mkdir ReleasePack\plugins
mkdir ReleasePack\plugins\X-Robot
mkdir ReleasePack\plugins\X-Robot\go-cqhttp
mkdir ReleasePack\plugins\X-Robot\qsign

echo "Copy Files..."

copy .\x64\Release\Manager.exe .\ReleasePack\
copy .\x64\Release\X-Robot.dll .\ReleasePack\plugins\
copy .\x64\Release\*.pdb .\ReleasePack\plugins\
copy .\Management\*.bat .\ReleasePack\
copy .\Management\yaml-cpp\Lib\yaml-cpp.dll .\ReleasePack\
copy .\Management\BDS-Deamon\BDS-Deamon.cmd .\ReleasePack\
copy .\Json\* .\ReleasePack\plugins\X-Robot\
copy .\go-cqhttp\* .\ReleasePack\plugins\X-Robot\go-cqhttp\
xcopy .\qsign\* .\ReleasePack\plugins\X-Robot\qsign\ /s
powershell Compress-Archive .\ReleasePack\* .\X-Robot.zip -Update
powershell rm .\ReleasePack\ -Recurse