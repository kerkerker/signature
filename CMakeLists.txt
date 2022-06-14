cmake_minimum_required(VERSION 3.19)

option(with-tsan "Build with tsan" OFF)
option(with-asan "Build with asan" OFF)
option(with-lsan "Build with lsan" OFF)
option(with-msan "Build with msan" OFF)
option(with-ubsan "Build with ubsan" OFF)
option(with-native "Build with native optimization" OFF)
option(with-tests "With tests" ON)

message(STATUS "  Build with thread sanitizer (with-tsan) ..... ${with-tsan}")
message(STATUS "  Build with address sanitizer (with-asan) .... ${with-asan}")
message(STATUS "  Build with leak sanitizer (with-lsan) ....... ${with-lsan}")
message(STATUS "  Build with memory sanitizer (with-msan) ..... ${with-msan}")
message(STATUS "  Build with ubsan sanitizer (with-ubsan) ..... ${with-ubsan}")
message(STATUS "  Build with native ........................... ${with-native}")
message(STATUS "  Build with tests (with-tests) ............... ${with-tests}")

project(signature)

set(BINARY_NAME signature)


set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options thread)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(spdlog REQUIRED)

set(SOURCE_FILES src/main.cpp src/Reader.cpp src/Writer.cpp src/Calculator.cpp src/Signature.cpp)

add_executable(${BINARY_NAME} ${SOURCE_FILES})
target_include_directories(${BINARY_NAME} PUBLIC ${Boost_INCLUDE_DIRS} include)
target_link_libraries(${BINARY_NAME} Boost::program_options Boost::thread OpenSSL::Crypto spdlog::spdlog)

if (with-native)
    target_compile_options(${BINARY_NAME} PUBLIC -march=native)
endif ()

if (with-asan AND NOT with-tsan)
    target_compile_options(${BINARY_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
    target_link_options(${BINARY_NAME} PRIVATE -fsanitize=address)
elseif (NOT with-asan AND with-tsan)
    target_compile_options(${BINARY_NAME} PRIVATE -fsanitize=thread)
    target_link_options(${BINARY_NAME} PRIVATE -fsanitize=thread)
elseif (with-asan AND with-tsan)
    message("Using ASAN and TSAN at the same time is not possible")
endif ()

if (with-lsan)
    target_compile_options(${BINARY_NAME} PRIVATE -fsanitize=leak)
    target_link_options(${BINARY_NAME} PRIVATE -fsanitize=leak)
endif ()

if (with-ubsan)
    target_compile_options(${BINARY_NAME} PRIVATE -fsanitize=undefined)
    target_link_options(${BINARY_NAME} PRIVATE -fsanitize=undefined -fsanitize=bounds)
endif ()

if (with-msan AND NOT with-tsan AND NOT with-asan AND NOT with-lsan)
    target_compile_options(${BINARY_NAME} PRIVATE -fsanitize=memory -fPIE
            -pie -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -O1 -fsanitize-memory-track-origins)
    target_link_options(${BINARY_NAME} PRIVATE -fsanitize=memory)
endif ()

if (with-tests)
    enable_testing()
    add_subdirectory(tests)
endif ()
