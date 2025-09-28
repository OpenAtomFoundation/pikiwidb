file(GLOB IAGENT_TEST_SRC CONFIGURE_DEPENDS "*.cpp")

add_executable(test_iagent ${IAGENT_TEST_SRC})

target_include_directories(test_iagent PUBLIC
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/iagent/include
)

target_link_libraries(test_iagent PUBLIC
    iagent_lib
    pthread
)

target_compile_definitions(test_iagent PRIVATE
    PROJECT_DIR="${CMAKE_SOURCE_DIR}/data/test"
    DEBUG=1
)

enable_testing()
add_test(NAME test_iagent COMMAND test_iagent)