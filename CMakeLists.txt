cmake_minimum_required(VERSION 3.23)

project(bnkextr)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -pedantic -Wpedantic -Wextra)

# add_compile_options(/await:strict)
add_compile_options(-fcoroutines -fconcepts-diagnostics-depth=5)
add_compile_definitions(BOOST_ASIO_HAS_IO_URING)

set(Boost_USE_DEBUG_LIBS $<$<CONFIG:Debug>:ON,OFF>)

find_package(
    Boost 1.82
    COMPONENTS
    system
    filesystem
    # url
    # json
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

# set_target_properties(bnkextr
#     PROPERTIES
#     CXX_STANDARD_REQUIRED YES
#     CXX_EXTENSIONS NO
#     )

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

# Static Runtime
# if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#     set_property(TARGET bnkextr PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# elseif ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND NOT APPLE)
#     target_link_options(bnkextr PRIVATE -static)
# endif ()

# Install executable
# install(TARGETS bnkextr
#     RUNTIME DESTINATION .)
