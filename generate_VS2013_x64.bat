mkdir "build"
cd build
set QTDIR=C:\Programming\lib\vs2013\Qt\Qt5.3.2\5.3\msvc2013_64_opengl
set PATH=%PATH%;%QTDIR%\bin
cmake -G "Visual Studio 12 Win64" ..
cd ..
pause