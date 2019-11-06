rem copy to local d:\bin
copy .\release\plot_cf_time_series.exe  d:\bin\plotcfts\plotcfts.exe
copy .\release\*.dll                    d:\bin\plotcfts\*.*
copy .\release\platforms\*.dll          d:\bin\plotcfts\platforms\*.*
copy .\doc\plotcfts_um.pdf              d:\bin\plotcfts\doc\plotcfts_um.pdf

rem copy to Bulletin
copy .\release\plot_cf_time_series.exe  n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\plotcfts.exe
copy .\release\*.dll                    n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\*.*
copy .\release\platforms\*.dll          n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\platforms\*.*
copy .\doc\plotcfts_um.pdf              n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\doc\plotcfts_um.pdf 
