rem For use with Windows
@echo off

mingw32-make -j4
if not exist "out\icarus.exe" (pause)

@echo on