cmake_minimum_required(VERSION 3.16)

project(IPCVcuBackend LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core SerialPort)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MOSQUITTO QUIET IMPORTED_TARGET libmosquitto)
endif()
if(NOT TARGET PkgConfig::MOSQUITTO)
    find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
    find_library(MOSQUITTO_LIBRARY NAMES mosquitto)
    if(MOSQUITTO_INCLUDE_DIR AND MOSQUITTO_LIBRARY)
        # Mosquitto 2.1 public headers include cJSON headers. Homebrew does not
        # expose that transitive include path when pkg-config is unavailable.
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

add_executable(ipc_vcu_backend
    include/DTCManager.h
    include/CanDatabase.h
    include/Protocol.h
    include/SerialManager.h
    include/SimulationManager.h
    include/SteeringInputManager.h
    include/VCUStateMachine.h
    include/VehicleModel.h
    src/main.cpp
    src/CanDatabase.cpp
    src/Protocol.cpp
    src/SerialManager.cpp
    src/SimulationManager.cpp
    src/SteeringInputManager.cpp
    src/VehicleModel.cpp
    src/VCUStateMachine.cpp
    src/DTCManager.cpp
)
if(TARGET PkgConfig::MOSQUITTO)
    target_link_libraries(ipc_vcu_backend PRIVATE PkgConfig::MOSQUITTO)
    target_compile_definitions(ipc_vcu_backend PRIVATE IPC_HAS_MOSQUITTO=1)
elseif(TARGET Mosquitto::Mosquitto)
    target_link_libraries(ipc_vcu_backend PRIVATE Mosquitto::Mosquitto)
    target_compile_definitions(ipc_vcu_backend PRIVATE IPC_HAS_MOSQUITTO=1)
else()
    message(WARNING "libmosquitto was not found; MQTT runtime support is disabled")
endif()

target_include_directories(ipc_vcu_backend PRIVATE include)
target_link_libraries(ipc_vcu_backend PRIVATE
    Qt5::Core
    Qt5::SerialPort
)
install(TARGETS ipc_vcu_backend RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)

    add_executable(ipc_vcu_tests
        include/DTCManager.h
        include/CanDatabase.h
        include/Protocol.h
        include/SteeringInputManager.h
        include/VCUStateMachine.h
        include/VehicleModel.h
        src/DTCManager.cpp
        src/CanDatabase.cpp
        src/Protocol.cpp
        src/VCUStateMachine.cpp
        src/VehicleModel.cpp
        src/SteeringInputManager.cpp
        tests/backend_tests.cpp
    )
    target_include_directories(ipc_vcu_tests PRIVATE include)
    target_link_libraries(ipc_vcu_tests PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME ipc_vcu_tests COMMAND ipc_vcu_tests)
    if(UNIX)
        add_executable(ipc_serial_tests
            tests/serial_tests.cpp
            include/SerialManager.h
            include/VehicleModel.h
            src/SerialManager.cpp src/Protocol.cpp src/CanDatabase.cpp
            src/VehicleModel.cpp src/DTCManager.cpp src/VCUStateMachine.cpp)
        target_include_directories(ipc_serial_tests PRIVATE include)
        target_link_libraries(ipc_serial_tests PRIVATE Qt5::Core Qt5::SerialPort)
        add_test(NAME ipc_serial_tests COMMAND ipc_serial_tests)
        set_tests_properties(ipc_serial_tests PROPERTIES TIMEOUT 20)
    endif()
    install(TARGETS ipc_vcu_tests RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
