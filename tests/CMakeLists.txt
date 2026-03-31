set(FM_TEST_SOURCES
    core/SimulationManagerTests.cpp
)

add_executable(fm_tests ${FM_TEST_SOURCES})
target_include_directories(fm_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(fm_tests PRIVATE fm_app Qt5::Test)

add_test(NAME fm_tests COMMAND fm_tests)