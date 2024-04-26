cmake_minimum_required(VERSION 3.16)

project(FitApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS
    #[[AndroidExtras]]
    # AndroidActivityView
    Charts
    Core
    Location
    Positioning
    Bluetooth
    Gui
    Qml
    Quick
)

# set(ANDROID_PACKAGE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/android CACHE INTERNAL "")

# FIT library
# set(FIT_DIR ../FitSDKRelease_21.115.00/cpp)
set(FIT_DIR ../FitSDKRelease_21.133.00/cpp)
file(GLOB FIT_SRC ${FIT_DIR}/*.hpp ${FIT_DIR}/*.cpp)
add_library(FIT_LIB ${FIT_SRC})
include_directories(${FIT_DIR})

if(${CMAKE_SYSTEM_NAME} STREQUAL Android)
    if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
        include_directories(C:/Qt/Tools/Android/Boost-for-Android/boost_1_84_0)
    else()
        include_directories(/home/x-ray/Android/Boost-for-Android/boost_1_84_0)
    endif()
else()
    # FIND BOOST
    find_package(Boost)
    if (Boost_FOUND)
        if(Boost_VERSION_MINOR LESS 84)
            message(FATAL_ERROR "==> !! Required Boost version is greater than or equal to 1.84.0! Current ${Boost_VERSION}.")
        endif()
        include_directories(${Boost_INCLUDE_DIR})
        message(==> "Boost ${Boost_VERSION} found!!! ${Boost_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Boost not found!!!")
    endif()
endif()


qt_standard_project_setup(REQUIRES 6.5)


qt_add_executable(appFitApp
    main.cpp
    fit.h
    fit.cpp
    bt/bluetoothbaseclass.cpp
    bt/bluetoothbaseclass.h
    bt/connectionhandler.cpp
    bt/connectionhandler.h
    bt/devicefinder.cpp
    bt/devicefinder.h
    bt/devicehandler.cpp
    bt/devicehandler.h
    bt/deviceinfo.cpp
    bt/deviceinfo.h
    bt/Dump
)

include_directories(bt)

qt_add_qml_module(appFitApp
    URI FitApp
    VERSION 1.0
    QML_FILES
        Main.qml
        Page1.qml
        Page2.qml
        Page3.qml
        Page4.qml
        Page5.qml
    SOURCES
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appFitApp PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appFitApp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android # Qt6

)



target_link_libraries(appFitApp PRIVATE
    Qt6::Core
    Qt6::Bluetooth
    Qt6::Charts
    # Qt6::CorePrivate
    Qt6::Location
    Qt6::Positioning
    Qt6::Qml
    Qt6::Quick
    FIT_LIB
)

include(GNUInstallDirs)
install(TARGETS appFitApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    get_filename_component(SHARED_PLIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../shared ABSOLUTE)
    if (IOS)
        set_target_properties(appFitApp PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.ios.plist"
        )
    else()
        set_target_properties(appFitApp PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()
