@echo off
SET _OLD_PATH=%PATH%
SET PATH=C:\msys32\mingw32\bin;C:\msys32\usr\bin
make
SET PATH=%_OLD_PATH%
