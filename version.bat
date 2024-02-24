@echo off
setlocal EnableDelayedExpansion
set file=%1

for /F "tokens=1-4 delims=N " %%a in (%file%) do (
   for %%A in (%%a %%b %%c %%d) do set "i=%%A"
)
set /a i=i+1
rem echo #define VERSION_BUILD %i% > %file% 