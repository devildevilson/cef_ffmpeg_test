cmake_minimum_required (VERSION 3.30)
include(FetchContent)

set(CMAKE_CONFIGURATION_TYPES
  Debug
  Release
  MinSizeRel
  RelWithDebInfo
  CACHE STRING "Build configurations" FORCE
)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(cef_ffmpeg_test)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(PROJECT_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_EXAMPLES OFF)
option(BUILD_TESTING OFF)

find_package(Threads REQUIRED)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.x
  GIT_PROGRESS   TRUE
  FIND_PACKAGE_ARGS
)

FetchContent_Declare(
  cpp-json
  GIT_REPOSITORY https://github.com/eteran/cpp-json.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
  FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(spdlog cpp-json)

if (WIN32)
  FetchContent_Declare(
    GLEW
    URL  https://github.com/nigels-com/glew/releases/download/glew-2.3.1/glew-2.3.1-win32.zip
    FIND_PACKAGE_ARGS
  )

  FetchContent_Declare(
    FFMPEG
    URL  https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-lgpl-shared.zip
    FIND_PACKAGE_ARGS
  )

  FetchContent_Declare(
    CEF
    URL  https://cef-builds.spotifycdn.com/cef_binary_147.0.10%2Bgd58e84d%2Bchromium-147.0.7727.118_windows64.tar.bz2
    #SOURCE_SUBDIR somePathThatNeverExists
  )

  FetchContent_MakeAvailable(GLEW FFMPEG CEF)

  set(CEF_ROOT ${FETCHCONTENT_BASE_DIR}/cef-src)
  list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)
  
  find_package(OpenGL REQUIRED)
else ()
  FetchContent_Declare(
    CEF
    #URL  https://cef-builds.spotifycdn.com/cef_binary_147.0.10%2Bgd58e84d%2Bchromium-147.0.7727.118_linux64.tar.bz2
    URL   "${CMAKE_CURRENT_LIST_DIR}/external/cef_binary.tar.bz2"
  )

  FetchContent_MakeAvailable(CEF)

  set(CEF_ROOT ${FETCHCONTENT_BASE_DIR}/cef-src)
  list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)

  find_package(OpenGL REQUIRED COMPONENTS EGL)
  find_package(GLEW REQUIRED)

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(AVCODEC REQUIRED libavcodec)
  pkg_check_modules(AVFORMAT REQUIRED libavformat)
  pkg_check_modules(SWSCALE REQUIRED libswscale)

  # CEF почему то очень хочет вот это
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  pkg_check_modules(GLIB REQUIRED glib-2.0)

  include_directories(${GTK3_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
endif ()

add_subdirectory(src/common/)
add_subdirectory(src/render/)
add_subdirectory(src/source/)

add_executable (${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} cef_ffmpeg_test::options cef_ffmpeg_test::common cef_ffmpeg_test::render cef_ffmpeg_test::source ${GTK3_LIBRARIES} ${GLIB_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIRECTORY}/bin")
if (LINUX)
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
  )
endif()

if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
  )
endif ()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_CURRENT_LIST_DIR}/resources 
    ${PROJECT_OUTPUT_DIRECTORY}/resources
)

SET_CEF_TARGET_OUT_DIR($<TARGET_FILE_DIR:${PROJECT_NAME}>)

copy_files("${PROJECT_NAME}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${PROJECT_OUTPUT_DIRECTORY}/bin")
copy_files("${PROJECT_NAME}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${PROJECT_OUTPUT_DIRECTORY}/bin")