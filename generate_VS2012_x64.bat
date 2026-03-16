mkdir "build"
cd build
set QTDIR=C:\Qt\Qt5.2.0\5.2.0\msvc2012_64_opengl
set PATH=%PATH%;%QTDIR%\bin
cmake -G "Visual Studio 11 Win64" ..
cd ..
pause