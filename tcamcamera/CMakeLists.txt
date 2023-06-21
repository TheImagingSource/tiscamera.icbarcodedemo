project(tcamcamera)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
pkg_check_modules(TCAMLIB tcam)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 COMPONENTS Widgets Core REQUIRED)


#if (Qt5Widgets_FOUND)
#    if (Qt5Widgets_VERSION VERSION_LESS 5.5)
#        message(FATAL_ERROR "Minimum supported Qt5 version is 5.5" ${Qt5_DIR} ${QT_QMAKE_EXECUTABLE})
#    endif()
#else()
#    message(SEND_ERROR "The Qt5Widgets library could not be found!")
#endif(Qt5Widgets_FOUND)

include_directories(tcamcamera 
                    ${CMAKE_CURRENT_LIST_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR} 
                    ${GSTREAMER_INCLUDE_DIRS} 
                    ${TCAM_INCLUDE_DIRS}
                    )



add_library( tcamcamera STATIC
            src/cdeviceselectiondlg.cpp 
            src/propertyctrl.cpp
            src/cpropertiesdialog.cpp 
            src/videoformat.cpp 
            src/videoformats.cpp
            src/cgrabber.cpp
)

target_include_directories(tcamcamera PUBLIC include )

