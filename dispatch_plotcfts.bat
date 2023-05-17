rem copy to local  "c:\Program Files\Deltares"  needed for QGIS
copy .\bin\x64\release\plotcfts.exe             "c:\Program Files\Deltares\plotcfts\plotcfts.exe"
copy .\bin\x64\release\*.dll                    "c:\Program Files\Deltares\plotcfts\*.*"
copy .\bin\x64\release\platforms\*.dll          "c:\Program Files\Deltares\plotcfts\platforms\*.*"
copy .\doc\plotcfts_um.pdf                      "c:\Program Files\Deltares\plotcfts\doc\plotcfts_um.pdf"

rem copy to Bulletin
copy .\bin\x64\release\plotcfts.exe             n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS_4.02.00\plotcfts.exe
copy .\bin\x64\release\*.dll                    n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS_4.02.00\*.*
copy .\bin\x64\release\platforms\*.dll          n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS_4.02.00\platforms\*.*
copy .\doc\plotcfts_um.pdf                      n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS_4.02.00\doc\plotcfts_um.pdf 
