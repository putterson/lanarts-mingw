set -e
mkdir -p mingw32-build ; cd mingw32-build
BUILD_OPTIMIZE=1 mingw32-cmake .. 
BUILD_OPTIMIZE=1 make -j5 lanarts
cd ..
rm -rf lanarts-pkg
mkdir lanarts-pkg
cd runtime
python compile_images.py > compiled/Resources.lua
cd ..
cp dlls/*.dll lanarts-pkg/
rm -rf lanarts-pkg/saves
cp -r runtime/* lanarts-pkg
cp mingw32-build/src/lanarts.exe lanarts-pkg/
strip lanarts-pkg/lanarts.exe
upx lanarts-pkg/lanarts.exe
cd lanarts-pkg
zip -r lanarts-controller.zip *
