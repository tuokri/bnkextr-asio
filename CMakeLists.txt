cmake_minimum_required(VERSION 3.25)

project(bnkextr_asio)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -pedantic -Wpedantic -Wextra)

# add_compile_options(/await:strict)
add_compile_options(-fcoroutines -fconcepts-diagnostics-depth=5)

add_compile_definitions(BOOST_ASIO_HAS_IO_URING)

set(Boost_USE_DEBUG_LIBS $<$<CONFIG:Debug>:ON,OFF>)
set(Boost_USE_MULTITHREADED ON)

find_package(
    Boost 1.82
    COMPONENTS
    system
    filesystem
    REQUIRED
)

add_library(
    bnkextr
    ${CMAKE_SOURCE_DIR}/src/bnkextr.cpp
)

add_executable(
    bnkextr_test
    ${CMAKE_SOURCE_DIR}/src/bnkextr.cpp
)

target_include_directories(
    bnkextr
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(
    bnkextr_test
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(
    bnkextr
    PUBLIC
    cxx_std_23
)

target_compile_features(
    bnkextr_test
    PUBLIC
    cxx_std_23
)

target_link_libraries(
    bnkextr
    PUBLIC
    Boost::system
    Boost::filesystem
    PRIVATE
    pthread
    uring
)

target_link_libraries(
    bnkextr_test
    PUBLIC
    Boost::system
    Boost::filesystem
    PRIVATE
    pthread
    uring
)
