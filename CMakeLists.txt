project(ICBarcode)
 
cmake_minimum_required(VERSION 3.1.12)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)

add_subdirectory(lib)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()


pkg_check_modules(GSTREAMER gstreamer-1.0 glib-2.0 gstreamer-base-1.0 gstreamer-plugins-base-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gobject-introspection-1.0)
pkg_check_modules(TCAMLIB tcam)
pkg_check_modules(JSONCPP jsoncpp)
pkg_check_modules(ICBARCODE ic_barcode)

message(STATUS "ICBARCODE LIB : " ${ICBARCODE_LIBRARIES}) 


find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
if (Qt5Widgets_FOUND)
    if (Qt5Widgets_VERSION VERSION_LESS 5.5)
        message(FATAL_ERROR "Minimum supported Qt5 version is 5.5" ${Qt5_DIR} ${QT_QMAKE_EXECUTABLE})
    endif()
else()
    message(SEND_ERROR "The Qt5Widgets library could not be found!")
endif(Qt5Widgets_FOUND)

pkg_check_modules(TCAM tcam)
if( TCAM_FOUND)
    message(STATUS "TCAM INC : " ${TCAM_INCLUDE_DIRS})
    message(STATUS "TCAM LIB : " ${TCAM_LIBRARIES}) 
else()
    message(NOTICE "tiscamera and its dev packages are not installed.")
    message(NOTICE "Please download and install tiscamera and the dev packages from https://github.com/TheImagingSource/tiscamera/releases")
    message(FATAL_ERROR "TCAM package missing.")
endif( TCAM_FOUND)




include_directories( 
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GSTREAMER_INCLUDE_DIRS} 
        ${JSONCPP_INCLUDE_DIRS}
        ${TCAM_INCLUDE_DIRS}
        ${FMT_INCLUDE_DIRS}
        tcamcamera/include
        )

add_definitions(${GSTREAMER_CFLAGS_OTHER})  

add_executable(ICBarcode 
                tcamcamera/src/propertyctrl.cpp
                tcamcamera/src/cpropertiesdialog.cpp 
                tcamcamera/src/videoformat.cpp 
                tcamcamera/src/videoformats.cpp
                tcamcamera/src/cdeviceselectiondlg.cpp 
                tcamcamera/src/cgrabber.cpp
                tcamcamera/src/customformatdlg.cpp 

                tcamcamera/include/propertyctrl.h
                tcamcamera/include/cpropertiesdialog.h 
                tcamcamera/include/videoformat.h 
                tcamcamera/include/videoformats.h
                tcamcamera/include/cdeviceselectiondlg.h 
                tcamcamera/include/cgrabber.h
                tcamcamera/include/customformatdlg.h 


                main.cpp 
                mainwindow.h 
                mainwindow.cpp 
                mainwindow.qrc
                mainwindow.ui
                )

target_link_libraries(ICBarcode 
                        ${TCAMLIB_LIBRARIES} 
                        ${JSONCPP_LIBRARIES} 
                        ${GSTREAMER_LIBRARIES} 
                        ${ICBARCODE_LIBRARIES}
                        Qt5::Widgets 
                        Qt5::Core
                        fmt
                        pthread
                        )



install(TARGETS ICBarcode RUNTIME DESTINATION bin)
