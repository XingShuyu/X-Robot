powershell rm .\X-Robot.zip
powershell sleep 1
echo "Making Folders..."
mkdir ReleasePack
mkdir ReleasePack\plugins
mkdir ReleasePack\plugins\X-Robot
mkdir ReleasePack\plugins\X-Robot\Gensokyo

echo "Copy Files..."

copy .\x64\Release\Manager.exe .\ReleasePack\
copy .\x64\Release\X-Robot.dll .\ReleasePack\plugins\
copy .\x64\Release\*.pdb .\ReleasePack\plugins\
copy .\Management\*.bat .\ReleasePack\
copy .\Management\yaml-cpp\Lib\yaml-cpp.dll .\ReleasePack\
copy .\Json\* .\ReleasePack\plugins\X-Robot\
copy .\Gensokyo\* .\ReleasePack\plugins\X-Robot\Gensokyo\
powershell Compress-Archive .\ReleasePack\* .\X-Robot.zip -Update
powershell rm .\ReleasePack\ -Recurse