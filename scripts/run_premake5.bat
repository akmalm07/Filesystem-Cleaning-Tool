@echo off

cd ..

setlocal

:: Set the root directory (parent folder)
set rootDir=%~dp0
set rootDir=%rootDir:~0,-1% 

for %%i in ("%rootDir%..") do set rootDir=%%~fi

echo Root directory (parent folder) is: %rootDir%

:: Ask the user for their IDE choice
echo Choose your IDE:
echo 1. Visual Studio
echo 2. GCC
set /p IDEChoice=Enter your choice (1 or 2): 

:: Create a temporary directory
set "tempDir=%temp%\delete_temp"
mkdir "%tempDir%"

:: Delete existing solution files
del /Q *.sln

:: Mirror the directory structure to the temp directory and delete unwanted files
rem Use robocopy to mirror the directory structure to the temp directory
robocopy "%rootDir%" "%tempDir%" *.vcxproj /S /NFL /NDL /NJH /NJS /NC /NS

rem Delete the files using robocopy
for /f "delims=" %%f in ('robocopy "%tempDir%" "%rootDir%" *.vcxproj /S /L /NFL /NDL /NJH /NJS /NC /NS ^| findstr /R /C:"^"') do del /Q "%%f"

rem Repeat for other project files (.vcxproj.user and .vcxproj.filters)
robocopy "%rootDir%" "%tempDir%" *.vcxproj.user /S /NFL /NDL /NJH /NJS /NC /NS
for /f "delims=" %%f in ('robocopy "%tempDir%" "%rootDir%" *.vcxproj.user /S /L /NFL /NDL /NJH /NJS /NC /NS ^| findstr /R /C:"^"') do del /Q "%%f"

robocopy "%rootDir%" "%tempDir%" *.vcxproj.filters /S /NFL /NDL /NJH /NJS /NC /NS
for /f "delims=" %%f in ('robocopy "%tempDir%" "%rootDir%" *.vcxproj.filters /S /L /NFL /NDL /NJH /NJS /NC /NS ^| findstr /R /C:"^"') do del /Q "%%f"

:: Clean up temp directory
rmdir /s /q "%tempDir%"

:: Generate new project files based on IDE choice
if "%IDEChoice%"=="1" (
    rem For Visual Studio, use premake for VS2022
    premake5 vs2022
) else if "%IDEChoice%"=="2" (
    rem For GCC, you can add the premake configuration for GCC here
    echo GCC option selected, running premake with GCC configuration.
    premake5 gmake
) else (
    echo Invalid choice. Please select either 1 or 2.
    exit /b
)

:: Wait for user input to close the window
pause