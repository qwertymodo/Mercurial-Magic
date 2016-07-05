rem For use with Windows
@echo off

mingw32-make -j4
if not exist "out\daedalus.exe" (pause)

@echo on