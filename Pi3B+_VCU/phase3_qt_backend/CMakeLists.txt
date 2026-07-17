cmake_minimum_required(VERSION 3.16)

project(IPCPhase3Backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 SerialPort)

add_executable(ipc_phase3_backend
    include/DTCManager.h
    include/Protocol.h
    include/SerialManager.h
    include/SimulationManager.h
    include/VCUStateMachine.h
    include/VehicleModel.h
    src/main.cpp
    src/SerialManager.cpp
    src/SimulationManager.cpp
    src/VehicleModel.cpp
    src/VCUStateMachine.cpp
    src/DTCManager.cpp
    qml.qrc
)

target_include_directories(ipc_phase3_backend PRIVATE include)
target_link_libraries(ipc_phase3_backend PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
    Qt5::SerialPort
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)

    add_executable(ipc_phase3_tests
        include/DTCManager.h
        include/Protocol.h
        include/VCUStateMachine.h
        include/VehicleModel.h
        src/DTCManager.cpp
        src/VCUStateMachine.cpp
        src/VehicleModel.cpp
        tests/backend_tests.cpp
    )
    target_include_directories(ipc_phase3_tests PRIVATE include)
    target_link_libraries(ipc_phase3_tests PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME ipc_phase3_tests COMMAND ipc_phase3_tests)
endif()
