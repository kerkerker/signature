
find_package(GTest REQUIRED)
add_executable(test_signature test.cpp ../src/Calculator.cpp ../src/Reader.cpp ../src/Signature.cpp ../src/Writer.cpp)
target_include_directories(test_signature PRIVATE ${CMAKE_SOURCE_DIR}/include ${GTEST_INCLUDE_DIRS})
target_link_libraries(test_signature
        GTest::GTest GTest::Main Boost::thread OpenSSL::Crypto spdlog::spdlog)

if (with-asan)
    target_compile_options(test_signature PRIVATE -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
    target_link_options(test_signature PRIVATE -fsanitize=address)
endif ()

if (with-tsan)
    target_compile_options(test_signature PRIVATE -fsanitize=thread)
    target_link_options(test_signature PRIVATE -fsanitize=thread)
endif ()


target_compile_options(test_signature PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
target_link_options(test_signature PUBLIC -fprofile-arcs -ftest-coverage)

gtest_discover_tests(test_signature
        XML_OUTPUT_DIR report)
