cmake_minimum_required(VERSION 3.14)
project(NovaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔍 Include src and include folders
include_directories(${CMAKE_SOURCE_DIR}/include)

# 🔍 Source files
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(novaengine ${SOURCES}
        include/ffmpeg_encoder.h
        src/ffmpeg_encoder.cpp
        include/slicer.hpp
        src/slicer.cpp
        include/udp_sender.hpp
        src/udp_sender.cpp
        include/packet_parser.hpp
        include/smart_collector.hpp
        src/receiver.cpp
        include/decode_and_display.hpp)

# ✅ OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(novaengine ${OpenCV_LIBS})

# ✅ PortAudio
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIB portaudio)

if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
    message(STATUS "PortAudio found")
    include_directories(${PORTAUDIO_INCLUDE_DIR})
    target_link_libraries(novaengine ${PORTAUDIO_LIB})
else()
    message(FATAL_ERROR "PortAudio not found")
endif()
find_library(AVCODEC_LIB avcodec)
find_library(AVUTIL_LIB avutil)
find_library(SWSCALE_LIB swscale)

if(AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
    target_link_libraries(novaengine ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
else()
    message(FATAL_ERROR "FFmpeg libraries not found")
endif()
set(CMAKE_BUILD_TYPE Debug)

# 🔁 Optional: libopus (uncomment if used)
# find_path(OPUS_INCLUDE_DIR opus/opus.h)
# find_library(OPUS_LIB opus)
# if(OPUS_INCLUDE_DIR AND OPUS_LIB)
#     include_directories(${OPUS_INCLUDE_DIR})
#     target_link_libraries(novaengine ${OPUS_LIB})
# endif()

# 🔁 Optional: SDL2 (for video rendering)
# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})
# target_link_libraries(novaengine ${SDL2_LIBRARIES})

# OS-specific settings (Linux)
if(UNIX)
    target_link_libraries(novaengine pthread dl)
endif()

# Output folder
set_target_properties(novaengine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

