@echo off
SET _OLD_PATH=%PATH%
SET PATH=%CD%\..\msys32\mingw32\bin;%CD%\..\msys32\usr\bin
make
SET PATH=%_OLD_PATH%
