cmake_minimum_required(VERSION 2.6.0)

find_package(ZLIB REQUIRED)
find_package(Qt4 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS thread REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(Boost COMPONENTS date_time REQUIRED)

include(FindOpenNI.cmake)

include(${QT_USE_FILE})
 
qt4_wrap_cpp(main_moc_SRCS
             main.h)

IF (UNIX)
	set(CMAKE_CXX_FLAGS "-O3 -msse2 -msse3")
ENDIF (UNIX)

include_directories(.
                    ../OpenNI)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ALL_DYN_LINK ON)   # force dynamic linking for all libraries

add_executable(Logger 
               main.cpp
               ${main_moc_SRCS}
               Logger.cpp
  OpenNI/openni_driver.cpp
  OpenNI/openni_device.cpp
  OpenNI/openni_exception.cpp
  OpenNI/openni_device_primesense.cpp
  OpenNI/openni_device_kinect.cpp
  OpenNI/openni_device_xtion.cpp
  OpenNI/openni_device_oni.cpp
  OpenNI/openni_image_yuv_422.cpp
  OpenNI/openni_image_bayer_grbg.cpp
  OpenNI/openni_image_rgb24.cpp
  OpenNI/openni_ir_image.cpp
  OpenNI/openni_depth_image.cpp
  )

target_link_libraries(Logger
                      ${ZLIB_LIBRARY}
                      ${Boost_LIBRARIES}
                      ${OPENNI_LIBRARY}
                      ${OpenCV_LIBS} 
                      ${QT_LIBRARIES})

