echo off

set exec=".\bin\x64\plotcfts.exe"
set netCDF_DIR=c:\Program Files\netCDF 4.9.2\
set PATH=c:\Qt\Qt6.10.2\6.10.2\msvc2022_64\;%PATH%
set PATH=c:\boost\Boost-1.90.0\;%PATH%

del %exec%
del /Q _build

copy packages\include\plot_cf_time_series_version.h.vcs packages\include\plot_cf_time_series_version.h 
copy packages\include\plot_cf_time_series_version.rc.vcs packages\include\plot_cf_time_series_version.rc 

echo on
cmake -B _build > cmake_configure.log 2>&1
cmake --build _build --verbose --config Release > cmake_build.log 2>&1

%exec%
