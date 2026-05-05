cmake_minimum_required (VERSION 3.30)

project (cef_ffmpeg_test_common)

add_library(cef_ffmpeg_test_options INTERFACE)
# похоже что CEF использует 20й стандарт
target_compile_features(cef_ffmpeg_test_options INTERFACE cxx_std_20)
target_compile_options(cef_ffmpeg_test_options INTERFACE
  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /ZI /Wall /wd5045 /wd4820 /wd4514 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /ZI /Wall /wd5045 /wd4820 /O2 /GR- /permissive- /Zc:preprocessor>>

  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -g -Wall -Wextra -fno-rtti -fno-inline>>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -Wall -Wextra -fno-rtti -pipe -flto=auto>>
  $<$<CONFIG:MinSizeRel>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -Wall -Wextra -fno-rtti -pipe -flto=auto -s>>
  $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:-mavx -O3 -g -Wall -Wextra -fno-rtti -pipe -flto=auto>>
)
target_compile_definitions(cef_ffmpeg_test_options INTERFACE
  $<$<CONFIG:Debug>:>
  $<$<CONFIG:Release>:NDEBUG _NDEBUG>
  $<$<CONFIG:MinSizeRel>:NDEBUG _NDEBUG>
  $<$<CONFIG:RelWithDebInfo>:>
)

add_library(cef_ffmpeg_test::options ALIAS cef_ffmpeg_test_options)

set(SRC_LIST 
  src/settings.cpp 
  src/utils.cpp
)

set(HDR_LIST 
  include/common/common.h
  include/common/settings.h
  include/common/source_interface.h
  include/common/utils.h
)

add_library(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>")
target_include_directories(${PROJECT_NAME} PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/common>")
target_link_libraries(${PROJECT_NAME} cpp-json spdlog::spdlog cef_ffmpeg_test::options)

add_library(cef_ffmpeg_test::common ALIAS ${PROJECT_NAME})