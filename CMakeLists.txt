cmake_minimum_required(VERSION 3.14)
project(NovaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(novaengine ${SOURCES})

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(novaengine ${OpenCV_LIBS})

# FFmpeg
find_library(AVCODEC_LIB avcodec)
find_library(AVUTIL_LIB avutil)
find_library(SWSCALE_LIB swscale)

if(AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
    target_link_libraries(novaengine ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
else()
    message(FATAL_ERROR "FFmpeg libraries not found")
endif()

# Linux sistem linkleri
if(UNIX)
    target_link_libraries(novaengine pthread dl)
endif()

# Derlenmiş dosyayı bin klasörüne at
set_target_properties(novaengine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
