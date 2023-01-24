@echo off
mkdir build
pushd build

REM NOTE: Windows requires `--config Release` at build time, whereas MacOS and Linux use `-DCMAKE_BUILD_TYPE=Release` at configure time

REM Configure debug (any configuration on Windows)
"C:\Program Files\CMake\bin\cmake.exe" .. -A x64

REM Configure release
REM "C:\Program Files\CMake\bin\cmake.exe" .. -A x64 -DCMAKE_BUILD_TYPE=Release

REM Build debug (configured configuration on MacOS and Linux)
"C:\Program Files\CMake\bin\cmake.exe" --build .

REM Build release
REM "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

popd
