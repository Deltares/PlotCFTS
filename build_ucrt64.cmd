echo off

set exec=".\bin\ucrt64\plotcfts.exe"
set netCDF_DIR=c:\msys64\ucrt64\lib\cmake\netCDF\
set PATH=c:\msys64\ucrt64\lib\cmake\Qt6\;%PATH%
set PATH=c:\msys64\ucrt64\lib\cmake\Boost-1.85.0\;%PATH%

del %exec%
del _build

copy packages\include\plot_cf_time_series_version.h.vcs packages\include\plot_cf_time_series_version.h 
copy packages\include\plot_cf_time_series_version.rc.vcs packages\include\plot_cf_time_series_version.rc 

echo on
cmake -B _build > cmake_configure.log 2>&1
cmake --build _build --verbose > cmake_build.log 2>&1

%exec%
