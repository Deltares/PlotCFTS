# PlotCFTS
Plot program to plot time series which are Climate and Forecast compliant.

To build the program PlotCTS you have to install QT 5.12 LTS and netCDF4.
The executable plot_cf_time_series.exe will placed in folder bin/x64/debug or bin/x64/release.
The batch file dispatch_plotcfts.bat is used to dispatch and rename the executable to plotcfts.exe

## Development environment
At this moment the environment environemnt is based on Visual Studio 2015

## Environment variables
Environment variables (example)
QT5DIR64=c:\Qt\Qt5.12.1\5.12.1\msvc2015_64\

## Link Libraries
netCDF library:
debug and release folder contain the same libraries
./lib/x64/release/netcdf.lib
./lib/x64/debug/netcdf.lib
                
## Runtime libraries
On the debug and release folder you need the runtime libraries of QT 5.12.1 and netcdf4.
Only release folder is given.
./release/platforms/qdirect2d.dll
                    qminimal.dll
                    qoffscreen.dll
                    qwindows.dll
./release/hdf.dll               
          hdf5.dll
          hdf5_cpp.dll
          hdf5_hl.dll
          hdf5_hl_cpp.dll
          hdf5_tools.dll
          jpeg.dll
          libcurl.dll
          mfhdf.dll
          msvcp140.dll
          netcdf.dll
          Qt5Core.dll
          Qt5Gui.dll
          Qt5PrintSupport.dll
          Qt5Widgets.dll
          style.qss1
          xdr.dll
          zlib1.dll


end document
