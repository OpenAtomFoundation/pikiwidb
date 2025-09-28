file(GLOB MOCK_TEST_SRC CONFIGURE_DEPENDS "*.cpp" "mock/*.cpp")
add_executable(test_mock ${MOCK_TEST_SRC})

target_include_directories(test_mock PUBLIC
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/mock/include
)

target_link_libraries(test_mock PUBLIC
    mock_lib
    pthread
)

target_compile_definitions(test_mock PRIVATE
    PROJECT_DIR="${CMAKE_SOURCE_DIR}/data/test"
    DEBUG=1
)

enable_testing()
add_test(NAME test_mock COMMAND test_mock)