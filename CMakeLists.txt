cmake_minimum_required(VERSION 3.20)

project(Todolist
    VERSION 1.0.0
    DESCRIPTION "A desktop todolist application with GitHub sync"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt 6.5+ packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Sql
    Charts
    Network
    Widgets
)

# Find nlohmann/json
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json 3.11 REQUIRED)
endif()

# Find libgit2 - try config first, then pkg-config fallback
find_package(PkgConfig QUIET)
find_package(libgit2 1.7 CONFIG QUIET)
if(libgit2_FOUND)
    if(TARGET libgit2::libgit2)
        set(TODOLIST_LIBGIT2_TARGET libgit2::libgit2)
    elseif(TARGET libgit2::libgit2package)
        set(TODOLIST_LIBGIT2_TARGET libgit2::libgit2package)
    endif()
endif()

if(NOT TODOLIST_LIBGIT2_TARGET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2>=1.7)
        set(TODOLIST_LIBGIT2_TARGET PkgConfig::LIBGIT2)
    else()
        find_package(libgit2 1.7 REQUIRED)
        if(TARGET libgit2::libgit2)
            set(TODOLIST_LIBGIT2_TARGET libgit2::libgit2)
        elseif(TARGET libgit2::libgit2package)
            set(TODOLIST_LIBGIT2_TARGET libgit2::libgit2package)
        else()
            message(FATAL_ERROR "libgit2 was found, but no known CMake target was exported")
        endif()
    endif()
endif()

# Fetch QAESEncryption for AES-256-GCM encryption
include(FetchContent)
FetchContent_Declare(
    qaesencryption
    GIT_REPOSITORY https://github.com/bricke/Qt-AES.git
    GIT_TAG master
)
FetchContent_MakeAvailable(qaesencryption)

# Platform-specific libraries
if(WIN32)
    # Windows: crypt32.lib for DPAPI
    find_library(CRYPT32_LIBRARY crypt32)
endif()

if(APPLE)
    # macOS: Security framework for Keychain
    find_library(SECURITY_FRAMEWORK Security)
endif()

# Include source directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/models
    ${CMAKE_SOURCE_DIR}/src/business
    ${CMAKE_SOURCE_DIR}/src/business/interfaces
    ${CMAKE_SOURCE_DIR}/src/data
)

# Collect source files
set(SOURCES
    src/main.cpp
    resources.qrc
)

# Glob all .cpp files from source directories
file(GLOB_RECURSE MODEL_SOURCES "src/models/*.cpp")
file(GLOB_RECURSE BUSINESS_SOURCES "src/business/*.cpp")
file(GLOB_RECURSE DATA_SOURCES "src/data/*.cpp")
file(GLOB_RECURSE HEADER_SOURCES "src/*.h")

list(APPEND SOURCES
    ${MODEL_SOURCES}
    ${BUSINESS_SOURCES}
    ${DATA_SOURCES}
    ${HEADER_SOURCES}
)

# Create executable
add_executable(Todolist ${SOURCES})

if(APPLE)
    set(TODOLIST_APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/assets/icons/app.icns")
    set_source_files_properties(${TODOLIST_APP_ICON_MACOS}
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(Todolist PRIVATE ${TODOLIST_APP_ICON_MACOS})
    set_target_properties(Todolist PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "app.icns")
endif()

# Set Qt QML import path
set(QML_IMPORT_PATH "${CMAKE_SOURCE_DIR}/src/qml" CACHE STRING "Qt QML import path")

# Link libraries
target_link_libraries(Todolist PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Charts
    Qt6::Network
    Qt6::Widgets
    nlohmann_json::nlohmann_json
    ${TODOLIST_LIBGIT2_TARGET}
    QtAES
)

# Platform-specific linking
if(WIN32 AND CRYPT32_LIBRARY)
    target_link_libraries(Todolist PRIVATE ${CRYPT32_LIBRARY})
endif()

if(APPLE AND SECURITY_FRAMEWORK)
    target_link_libraries(Todolist PRIVATE ${SECURITY_FRAMEWORK})
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS Todolist
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "Todolist")
set(CPACK_PACKAGE_VENDOR "Todolist Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A desktop todolist application with GitHub sync")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "Todolist")
    set(CPACK_NSIS_DISPLAY_NAME "Todolist ${PROJECT_VERSION}")
    set(CPACK_NSIS_CONTACT "support@todolist.local")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Todolist ${PROJECT_VERSION}")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/assets/dmg_setup.scpt")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/icons/app.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Todolist")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "Todolist Version ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE "app.icns")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.todolist.app")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2026 Todolist Team")
endif()

include(CPack)

# Add tests subdirectory when present
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
