message("********************************") 
message("*** Enter packages directory ***") 
message("********************************") 

set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)

set(exe_name PlotCFTS)

# include_directories(include)
set(headers_files
    include/mainwindow.h
    include/qcustomplot.h
    include/cf_time_series.h
    include/netcdf.h
    include/program_arguments.h
    include/perf_timer.h
    include/read_json.h
    include/time_series_plot.h
    include/plot_cf_time_series_version.h
#    include/plot_cf_time_series_version.rc
    include/resource.h
)    

set(no_group_source_files
    "include/version_number.ini"
)

set(source_files
    src/cf_timeseries.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/perf_timer.cpp
    src/plot_cf_time_series_version.cpp
    src/qcustomplot.cpp
    src/read_json.cpp
    src/time_series_plot.cpp
)

qt_add_executable(${exe_name} ${source_files} ${headers_files})

# Combine the two custom target definitions into one
add_custom_target(
    myHeaderTarget
    COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}\\..\\scripts\\git_insert_hash.ps1" ..\\..\\packages\\include plot_cf_time_series_version.h
    COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}\\..\\scripts\\git_insert_hash.ps1" ..\\..\\packages\\include plot_cf_time_series_version.rc
)

# Ensure myHeaderTarget runs before the executable is built
add_dependencies(${exe_name} myHeaderTarget)

if (MSVC)
    message("*** MSVC ")
    target_include_directories(${exe_name} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/."
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "c:/Qt/Qt6.10.3/6.10.3/msvc2022_64/"
        "c:/boost/boost_1_90_0/"
    )
    message("--- MSVC ")
else()
    message("*** UCRT64 ")
    target_include_directories(${exe_name} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/."
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "c:/msys64/ucrt64/lib"
        "c:/msys64/ucrt64/include/boost/"
    )
    message("--- UCRT64 ")
endif()

#target_link_libraries(${exe_name} PRIVATE Qt6::EntryPoint)
target_link_libraries(${exe_name} PRIVATE Qt6::Core)
target_link_libraries(${exe_name} PRIVATE Qt6::Gui)
target_link_libraries(${exe_name} PRIVATE Qt6::Widgets)
target_link_libraries(${exe_name} PRIVATE Qt6::PrintSupport)
if (MSVC)
    target_link_libraries(${exe_name} PRIVATE "c:/Program Files/netCDF 4.9.2/lib/netcdf.lib")
else()
    target_link_libraries(${exe_name} PRIVATE netcdf)
endif()

set_property(TARGET ${exe_name} PROPERTY WIN32_EXECUTABLE true)

set(install_dir ${CMAKE_BINARY_DIR})
set(build_dir ${CMAKE_BINARY_DIR})
install(TARGETS ${exe_name} DESTINATION .)
if (MSVC)
    install(FILES 
        ${Qt6_DIR}/../../../bin/Qt6Core.dll
        ${Qt6_DIR}/../../../bin/Qt6Gui.dll
        ${Qt6_DIR}/../../../bin/Qt6Widgets.dll
        ${Qt6_DIR}/../../../bin/Qt6PrintSupport.dll 
    DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY ${Qt6_DIR}/../../../plugins/platforms/ DESTINATION ${CMAKE_INSTALL_PREFIX}/platforms)
    install(DIRECTORY "c:/Program Files/netCDF 4.9.2/bin/"
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            FILES_MATCHING PATTERN "*.dll")
endif()

message("*** Exiting packages directory")