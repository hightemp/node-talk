# NodeTalk test suite
#
# Each test is an independent Qt Test executable so failures are
# isolated and parallel test runs (`ctest -j`) work.

set(NODETALK_TESTS
    tst_protocol
    tst_database
    tst_identity
    tst_filetransfer
    tst_smoke
)

foreach(TEST_NAME ${NODETALK_TESTS})
    qt_add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE
        nodetalk_core
        Qt6::Test
        Qt6::Network
        Qt6::Sql
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 60
    )
endforeach()

# tst_smoke pulls in widgets so it can spin up MainWindow headless
target_link_libraries(tst_smoke PRIVATE nodetalk_ui Qt6::Widgets)
