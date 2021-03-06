rem copy to local d:\bin
copy .\bin\x64\release\plotcfts.exe             d:\bin\plotcfts\plotcfts.exe
copy .\bin\x64\release\*.dll                    d:\bin\plotcfts\*.*
copy .\bin\x64\release\platforms\*.dll          d:\bin\plotcfts\platforms\*.*
copy .\doc\plotcfts_um.pdf                      d:\bin\plotcfts\doc\plotcfts_um.pdf

rem copy to local  "c:\Program Files\Deltares"  needed for QGIS
copy .\bin\x64\release\plotcfts.exe             "c:\Program Files\Deltares\plotcfts\plotcfts.exe"
copy .\bin\x64\release\*.dll                    "c:\Program Files\Deltares\plotcfts\*.*"
copy .\bin\x64\release\platforms\*.dll          "c:\Program Files\Deltares\plotcfts\platforms\*.*"
copy .\doc\plotcfts_um.pdf                      "c:\Program Files\Deltares\plotcfts\doc\plotcfts_um.pdf"

rem copy to Bulletin
copy .\bin\x64\release\plotcfts.exe             n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\plotcfts.exe
copy .\bin\x64\release\*.dll                    n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\*.*
copy .\bin\x64\release\platforms\*.dll          n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\platforms\*.*
copy .\doc\plotcfts_um.pdf                      n:\Deltabox\Bulletin\mooiman\programs\PlotCFTS\doc\plotcfts_um.pdf 
