add_executable(cms_string_tests
    ../test/test_cmsString.cpp
)

target_link_libraries(cms_string_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_string_tests PRIVATE support)
cms_configure_target(cms_string_tests)

add_test(NAME cms.string COMMAND cms_string_tests)
set_tests_properties(cms.string PROPERTIES TIMEOUT 30)

add_executable(cms_logger_tests
    ../test/test_cmsLogger.cpp
)

target_link_libraries(cms_logger_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_logger_tests PRIVATE support)
cms_configure_target(cms_logger_tests)

add_test(NAME cms.logger COMMAND cms_logger_tests)
set_tests_properties(cms.logger PROPERTIES TIMEOUT 30)

add_executable(cms_test_harness_probe
    support/test_harness_probe.cpp
)

target_include_directories(cms_test_harness_probe PRIVATE support)
cms_configure_target(cms_test_harness_probe)

add_test(
    NAME cms.test_harness.success
    COMMAND cms_test_harness_probe success
)

add_test(
    NAME cms.test_harness.check_failure
    COMMAND cms_test_harness_probe check_failure
)

add_test(
    NAME cms.test_harness.require_failure
    COMMAND cms_test_harness_probe require_failure
)

add_test(
    NAME cms.test_harness.zero
    COMMAND cms_test_harness_probe zero
)

set_tests_properties(
    cms.test_harness.check_failure
    cms.test_harness.require_failure
    cms.test_harness.zero
    PROPERTIES WILL_FAIL TRUE
)
