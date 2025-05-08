cmake_minimum_required(VERSION 3.30.0)

project(PlotCFTS VERSION 4.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
    message("*** MSVC ")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../bin/x64)
    set(CMAKE_CXX_COMPILER cl)
    set(CMAKE_BUILD_TYPE Release)
else()
    message("*** UCRT64 ")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../bin/ucrt64)
    set(CMAKE_CXX_COMPILER g++)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
if (Qt6_FOUND)
    message("*** Qt6 found")
else()
    message("*** Qt6 not found")
endif()

find_package(Boost 1.85.0 REQUIRED COMPONENTS
             date_time filesystem iostreams)
if(Boost_FOUND)
    message("*** Boost found")
else()
    message("*** Boost not found")
endif()

find_package(netCDF 4.9.2 REQUIRED)
if(netCDF_FOUND)
    message("*** netCDF found")
    message(STATUS "    ${netCDF_LIBRARIES}")
    message(STATUS "    ${netCDF_INCLUDE_DIR}")
else()
    message("*** netCDF not found")
endif()

add_subdirectory(packages)

