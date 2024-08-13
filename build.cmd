del .\bin\ucrt64\plotcfts.exe
copy packages\include\plot_cf_time_series_version.h.vcs packages\include\plot_cf_time_series_version.h 
copy packages\include\plot_cf_time_series_version.rc.vcs packages\include\plot_cf_time_series_version.rc 
del _build

echo off
set netCDF_DIR=c:\msys64\ucrt64\lib\cmake\netCDF\
set PATH=c:\msys64\ucrt64\lib\cmake\Qt6\;%PATH%
set PATH=c:\msys64\ucrt64\lib\cmake\Boost-1.85.0\;%PATH%

echo on
cmake -B _build > cmake_configure.log 2>&1
cmake --build _build --verbose --config Release > cmake_build.log 2>&1

.\bin\ucrt64\plotcfts.exe
