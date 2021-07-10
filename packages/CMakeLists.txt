set(PROJECT_NAME PlotCFTS)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "include/version_number.ini"
)
source_group("" FILES ${no_group_source_files})

#set(Generated_Files
#    "generated/moc_mainwindow.cpp"
#    "generated/moc_qcustomplot.cpp"
#    "generated/moc_time_series_plot.cpp"
#)
source_group("Generated Files" FILES ${Generated_Files})

set(Header_Files
    "include/cf_time_series.h"
    "include/mainwindow.h"
    "include/program_arguments.h"
    "include/qcustomplot.h"
    "include/read_json.h"
    "include/time_series_plot.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "src/cf_timeseries.cpp"
    "src/main.cpp"
    "src/mainwindow.cpp"
    "src/plot_cf_time_series_version.cpp"
    "src/qcustomplot.cpp"
    "src/read_json.cpp"
    "src/time_series_plot.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${no_group_source_files}
    ${Generated_Files}
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE interaction-example)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Qt4VSv1.0"
)
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "plotcfts"
    TARGET_NAME_RELEASE "plotcfts"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/$ENV{platform}/$<CONFIG>"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/$ENV{platform}/$<CONFIG>"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.;"
    "${CMAKE_CURRENT_SOURCE_DIR}/include;"
    "$ENV{QT5DIR64}/include;"
    "$ENV{BOOST_INCLUDEDIR}"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "QT_NO_DEBUG;"
        "NDEBUG"
    ">"
    "_WINDOWS;"
    "UNICODE;"
    "WIN64"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /RTC1;
            /Zi
        >
        $<$<CONFIG:Release>:
            /O2;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            -Zc:strictStrings
        >
        /W4;
        /GR;
        -w34100;
        -w34189;
        -w44996;
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Zc:wchar_t
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG
        >
        $<$<CONFIG:Release>:
            /INCREMENTAL:NO
        >
        /NOLOGO;
        /SUBSYSTEM:WINDOWS;
        /NXCOMPAT;
        /DYNAMICBASE;
        "/MANIFESTDEPENDENCY:type='win32';
        name='Microsoft.Windows.Common-Controls';
        version='6.0.0.0';
        publicKeyToken='6595b64144ccf1df';
        language='*';
        processorArchitecture='*'"
    )
endif()

################################################################################
# Pre build events
################################################################################
add_custom_command_if(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMANDS
    COMMAND   $<CONFIG:Debug> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.h
    COMMAND   $<CONFIG:Debug> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.rc
    COMMAND $<CONFIG:Release> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.h
    COMMAND $<CONFIG:Release> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.rc
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
        "qtmaind;"
        "Qt5Cored;"
        "Qt5Guid;"
        "Qt5Widgetsd;"
        "Qt5PrintSupportd"
    ">"
    "$<$<CONFIG:Release>:"
        "qtmain;"
        "Qt5Core;"
        "Qt5Gui;"
        "Qt5Widgets;"
        "Qt5PrintSupport"
    ">"
    "netcdf"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "$ENV{QT5DIR64}/lib;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>"
)

