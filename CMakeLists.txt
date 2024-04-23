cmake_minimum_required(VERSION 3.16)

project(FitApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Charts Positioning Location)

# FIT library
set(FIT_DIR ../FitSDKRelease_21.115.00/cpp)
include_directories(${FIT_DIR})

# FIND BOOST
find_package(Boost)
if (Boost_FOUND)
    if(Boost_VERSION_MINOR LESS 84)
        message(FATAL_ERROR "==> !! Required Boost version is greater than or equal to 1.84.0! Current ${Boost_VERSION}.")
    endif()
    include_directories(${Boost_INCLUDE_DIR})
    message(==> "Boost ${Boost_VERSION} found!!!")
else()
    message(FATAL_ERROR "Boost not found!!!")
endif()


qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appFitApp    main.cpp fit.h fit.cpp)

qt_add_qml_module(appFitApp
    URI FitApp
    VERSION 1.0
    QML_FILES
        Main.qml
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
)

# FIT library
file(GLOB FIT_SRC ${FIT_DIR}/*.hpp ${FIT_DIR}/*.cpp)
# include_directories(${FIT_DIR})
add_library(FIT_LIB ${FIT_SRC})

target_link_libraries(appFitApp PRIVATE
    Qt6::Charts
    Qt6::Positioning
    Qt6::Location
    Qt6::Quick
    FIT_LIB
)

include(GNUInstallDirs)
install(TARGETS appFitApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
