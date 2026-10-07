cmake_minimum_required(VERSION 3.16)

project(IPCCluster LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Qml Quick SerialPort)

set(IPC_BACKEND_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vcu/cpp_backend"
    CACHE PATH "Path to the IPC C++ backend")

if(NOT EXISTS "${IPC_BACKEND_ROOT}/include/VehicleModel.h")
    message(FATAL_ERROR
        "Backend not found. Set IPC_BACKEND_ROOT to IPC_Project/vcu/cpp_backend")
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MOSQUITTO QUIET IMPORTED_TARGET libmosquitto)
endif()

if(NOT TARGET PkgConfig::MOSQUITTO)
    find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
    find_library(MOSQUITTO_LIBRARY NAMES mosquitto)
    if(MOSQUITTO_INCLUDE_DIR AND MOSQUITTO_LIBRARY)
        find_path(CJSON_INCLUDE_DIR cjson/cJSON.h)
        set(MOSQUITTO_INCLUDE_DIRS "${MOSQUITTO_INCLUDE_DIR}")
        if(CJSON_INCLUDE_DIR)
            list(APPEND MOSQUITTO_INCLUDE_DIRS "${CJSON_INCLUDE_DIR}")
        endif()

        add_library(Mosquitto::Mosquitto UNKNOWN IMPORTED)
        set_target_properties(Mosquitto::Mosquitto PROPERTIES
            IMPORTED_LOCATION "${MOSQUITTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MOSQUITTO_INCLUDE_DIRS}")
    endif()
endif()

set(BACKEND_SOURCES
    ${IPC_BACKEND_ROOT}/src/CanDatabase.cpp
    ${IPC_BACKEND_ROOT}/src/Protocol.cpp
    ${IPC_BACKEND_ROOT}/src/SerialManager.cpp
    ${IPC_BACKEND_ROOT}/src/SimulationManager.cpp
    ${IPC_BACKEND_ROOT}/src/SteeringInputManager.cpp
    ${IPC_BACKEND_ROOT}/src/VehicleModel.cpp
    ${IPC_BACKEND_ROOT}/src/DTCManager.cpp
    ${IPC_BACKEND_ROOT}/src/VCUStateMachine.cpp
)

add_executable(ipc_cluster
    main.cpp
    BackendBridge.cpp
    BackendBridge.h
    DistanceMetrics.cpp
    DistanceMetrics.h
    radialbar.cpp
    radialbar.h
    qml.qrc
    ${IPC_BACKEND_ROOT}/include/SerialManager.h
    ${IPC_BACKEND_ROOT}/include/SimulationManager.h
    ${IPC_BACKEND_ROOT}/include/SteeringInputManager.h
    ${BACKEND_SOURCES}
)

target_include_directories(ipc_cluster PRIVATE
    ${IPC_BACKEND_ROOT}/include
)

target_link_libraries(ipc_cluster PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::SerialPort
)

if(TARGET PkgConfig::MOSQUITTO)
    target_link_libraries(ipc_cluster PRIVATE PkgConfig::MOSQUITTO)
    target_compile_definitions(ipc_cluster PRIVATE IPC_HAS_MOSQUITTO=1)
elseif(TARGET Mosquitto::Mosquitto)
    target_link_libraries(ipc_cluster PRIVATE Mosquitto::Mosquitto)
    target_compile_definitions(ipc_cluster PRIVATE IPC_HAS_MOSQUITTO=1)
else()
    message(WARNING "libmosquitto not found; ESP32 keypad input is disabled")
endif()

if(BUILD_TESTING)
    add_executable(ipc_menu_tests tests/menu_tests.cpp)
    target_compile_definitions(ipc_menu_tests PRIVATE IPC_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
    target_link_libraries(ipc_menu_tests PRIVATE Qt5::Core Qt5::Qml Qt5::Gui)
    add_test(NAME ipc_menu_tests COMMAND ipc_menu_tests)
    set_tests_properties(ipc_menu_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(ipc_distance_tests tests/distance_tests.cpp DistanceMetrics.cpp)
    target_include_directories(ipc_distance_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ipc_distance_tests PRIVATE Qt5::Core)
    add_test(NAME ipc_distance_tests COMMAND ipc_distance_tests)
    add_executable(ipc_cluster_tests
        tests/ipc_cluster_tests.cpp
        ${IPC_BACKEND_ROOT}/src/CanDatabase.cpp
        ${IPC_BACKEND_ROOT}/src/Protocol.cpp
        ${IPC_BACKEND_ROOT}/src/VehicleModel.cpp
        ${IPC_BACKEND_ROOT}/src/DTCManager.cpp
        ${IPC_BACKEND_ROOT}/src/VCUStateMachine.cpp
    )
    target_include_directories(ipc_cluster_tests PRIVATE
        ${IPC_BACKEND_ROOT}/include
    )
    target_link_libraries(ipc_cluster_tests PRIVATE Qt5::Core)
    add_test(NAME ipc_cluster_tests COMMAND ipc_cluster_tests)
endif()
