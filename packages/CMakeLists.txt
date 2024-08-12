message("   Enter packages directory") 

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
add_custom_target(
    myHeaderTarget
    COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}\\..\\scripts\\git_insert_hash.cmd ..\\..\\packages\\include plot_cf_time_series_version.h 
    )
add_dependencies(${exe_name} myHeaderTarget)
add_custom_command(
    TARGET ${exe_name}
    PRE_BUILD
    COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}\\..\\scripts\\git_insert_hash.cmd ..\\..\\packages\\include plot_cf_time_series_version.rc 
    )

if (MSVC)
    message("*** MSVC ")
    target_include_directories(${exe_name} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/."
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "c:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.40.33807/include/"
        "c:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt/"
        "c:/Qt/Qt6.7.2/6.7.2/msvc2019_64"
        "c:/boost/boost_1_85_0/"
    )
else()
    message("*** UCRT64 ")
    target_include_directories(${exe_name} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/."
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "c:/msys64/ucrt64/include/Qt6/"
        "c:/msys64/ucrt64/include/boost/"
    )
endif()

target_link_libraries(${exe_name} PRIVATE Qt6::Core)
target_link_libraries(${exe_name} PRIVATE Qt6::Gui)
target_link_libraries(${exe_name} PRIVATE Qt6::Widgets)
target_link_libraries(${exe_name} PRIVATE Qt6::PrintSupport)
target_link_libraries(${exe_name} PRIVATE "c:/Program Files/netCDF 4.9.2/lib/netcdf.lib")

set(install_dir ${CMAKE_BINARY_DIR})
set(build_dir ${CMAKE_BINARY_DIR})
install(TARGETS ${exe_name} RUNTIME DESTINATION bin/Release)

message("   Exiting packages directory") 
