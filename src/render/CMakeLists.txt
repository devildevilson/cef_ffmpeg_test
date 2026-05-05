cmake_minimum_required (VERSION 3.30)

project (cef_ffmpeg_test_render)

if (WIN32)
  add_library(GLEW::GLEW SHARED IMPORTED GLOBAL)
  set_target_properties(GLEW::GLEW PROPERTIES
    IMPORTED_LOCATION
      "${FETCHCONTENT_BASE_DIR}/glew-src/bin/Release/x64/glew32.dll"
    IMPORTED_IMPLIB
      "${FETCHCONTENT_BASE_DIR}/glew-src/lib/Release/x64/glew32.lib"
    INTERFACE_INCLUDE_DIRECTORIES
      "${FETCHCONTENT_BASE_DIR}/glew-src/include"
  )
endif()

set(SRC_LIST 
  src/resource_manager.cpp
 "src/system.cpp" "src/window.cpp")

set(HDR_LIST 
  include/render/resource_manager.h
  include/render/system.h
 "include/render/window.h")

add_library(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>")
target_include_directories(${PROJECT_NAME} PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/render>")
target_link_libraries(${PROJECT_NAME} cef_ffmpeg_test::options cef_ffmpeg_test::common OpenGL::GL GLEW::GLEW)
if (NOT WIN32)
  target_link_libraries(${PROJECT_NAME} EGL)
endif()

add_library(cef_ffmpeg_test::render ALIAS ${PROJECT_NAME})