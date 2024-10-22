cmake_minimum_required(VERSION 3.14)

project(zmq_rec_rep VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")

find_package(ZeroMQ REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${cppzmq_INCLUDE_DIR}
)

add_executable(zmq_recorder
    parse_cfg.cpp
    recorder.cpp
)

target_link_libraries(zmq_recorder
    pthread
    libzmq-static
)

add_executable(zmq_replayer
    parse_cfg.cpp
    replayer.cpp
)

target_link_libraries(zmq_replayer
    pthread
    libzmq-static
)

install(TARGETS zmq_recorder RUNTIME DESTINATION bin)
install(TARGETS zmq_replayer RUNTIME DESTINATION bin)