cmake_minimum_required (VERSION 3.30)

project (cef_ffmpeg_test_source)

if (WIN32)
  add_library(FFMPEG::AVCODEC SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::AVDEVICE SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::AVFILTER SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::AVFORMAT SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::AVUTIL SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::SWRESAMPLE SHARED IMPORTED GLOBAL)
  add_library(FFMPEG::SWSCALE SHARED IMPORTED GLOBAL)

  set_target_properties(FFMPEG::AVCODEC PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/avcodec-62.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/avcodec.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::AVDEVICE PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/avdevice-62.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/avdevice.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::AVFILTER PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/avfilter-11.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/avfilter.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::AVFORMAT PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/avformat-62.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/avformat.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::AVUTIL PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/avutil-60.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/avutil.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::SWRESAMPLE PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/swresample-6.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/swresample.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  set_target_properties(FFMPEG::SWSCALE PROPERTIES
    IMPORTED_LOCATION "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin/swscale-9.dll"
    IMPORTED_IMPLIB "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/lib/swscale.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FETCHCONTENT_BASE_DIR}/ffmpeg-src/include"
  )

  add_library(FFMPEG INTERFACE IMPORTED GLOBAL)
  set_property(TARGET FFMPEG PROPERTY
    INTERFACE_LINK_LIBRARIES FFMPEG::AVCODEC FFMPEG::AVDEVICE FFMPEG::AVFILTER FFMPEG::AVFORMAT FFMPEG::AVUTIL FFMPEG::SWRESAMPLE FFMPEG::SWSCALE
  )

else()
  pkg_check_modules(AVCODEC REQUIRED libavcodec)
  pkg_check_modules(AVDEVICE REQUIRED libavdevice)
  pkg_check_modules(AVFILTER REQUIRED libavfilter)
  pkg_check_modules(AVFORMAT REQUIRED libavformat)
  pkg_check_modules(AVUTIL REQUIRED libavutil)
  pkg_check_modules(SWRESAMPLE REQUIRED libswresample)
  pkg_check_modules(SWSCALE REQUIRED libswscale)

  add_library(FFMPEG INTERFACE IMPORTED GLOBAL)
  set_property(TARGET FFMPEG PROPERTY
    INTERFACE_LINK_LIBRARIES ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${SWSCALE_LIBRARIES} ${AVUTIL_LIBRARIES}
  )
endif()

set(SRC_LIST 
  src/ffmpeg.cpp
  src/cef.cpp
)

set(HDR_LIST 
  include/source/ffmpeg.h
  include/source/cef.h
)

message(STATUS "CEF_STANDARD_LIBS content: ${CEF_STANDARD_LIBS}")
message(STATUS "CEF_INCLUDE_PATH content: ${CEF_INCLUDE_PATH}")

add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

add_library(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>")
target_include_directories(${PROJECT_NAME} PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/source>")
add_dependencies(${PROJECT_NAME} libcef_dll_wrapper)
target_link_libraries(${PROJECT_NAME} cef_ffmpeg_test::common cef_ffmpeg_test::options cef_ffmpeg_test::render FFMPEG libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE "${CEF_INCLUDE_PATH}")

add_library(cef_ffmpeg_test::source ALIAS ${PROJECT_NAME})