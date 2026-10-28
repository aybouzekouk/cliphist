cmake_minimum_required(VERSION 3.21)

project(Cliphist VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If system dev headers are missing, use a locally extracted prefix that
# provides X11 + OpenGL headers and linker symlinks (see README). Installing
# libx11-dev libxtst-dev libgl-dev libglx-dev makes this prefix unnecessary.
if(EXISTS "$ENV{HOME}/.local/x11dev")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/x11dev")
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Gui Widgets)
qt_standard_project_setup()

find_package(X11 REQUIRED)

qt_add_executable(cliphist
    src/main.cpp
    src/clipboardmanager.cpp
    src/clipboardmanager.h
    src/globalhotkey.cpp
    src/globalhotkey.h
    src/pastehelper.cpp
    src/pastehelper.h
    src/theme.cpp
    src/theme.h
)

qt_add_qml_module(cliphist
    URI Cliphist
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ClipEntryDelegate.qml
)

target_link_libraries(cliphist PRIVATE
    Qt6::Quick
    Qt6::Gui
    Qt6::Widgets
    X11::X11
    X11::Xtst
)
