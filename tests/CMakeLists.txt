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

add_library(cms_status_compile_contract OBJECT
    compile/test_status_contract.cpp
)

target_link_libraries(cms_status_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_status_compile_contract)

add_library(cms_status_header_compile OBJECT
    compile/test_status_header.cpp
)

target_compile_features(cms_status_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_status_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_status_header_compile)

add_executable(cms_status_tests
    unit/test_status.cpp
)

target_link_libraries(cms_status_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_status_tests PRIVATE support)
cms_configure_target(cms_status_tests)

add_test(NAME cms.status COMMAND cms_status_tests)
set_tests_properties(cms.status PROPERTIES TIMEOUT 30)

add_library(cms_string_view_compile_contract OBJECT
    compile/test_string_view_contract.cpp
)

target_link_libraries(cms_string_view_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_string_view_compile_contract)

add_library(cms_string_buffer_compile_contract OBJECT
    compile/test_string_buffer_contract.cpp
)

target_link_libraries(cms_string_buffer_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_string_buffer_compile_contract)

add_library(cms_string_view_header_compile OBJECT
    compile/test_string_view_header.cpp
)

target_compile_features(cms_string_view_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_string_view_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_string_view_header_compile)

add_library(cms_string_buffer_header_compile OBJECT
    compile/test_string_buffer_header.cpp
)

target_compile_features(cms_string_buffer_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_string_buffer_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_string_buffer_header_compile)

add_executable(cms_string_view_tests
    unit/test_string_view.cpp
)

target_link_libraries(cms_string_view_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_string_view_tests PRIVATE support)
cms_configure_target(cms_string_view_tests)

add_test(NAME cms.string_view COMMAND cms_string_view_tests)
set_tests_properties(cms.string_view PROPERTIES TIMEOUT 30)

add_executable(cms_string_buffer_tests
    unit/test_string_buffer.cpp
)

target_link_libraries(cms_string_buffer_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_string_buffer_tests PRIVATE support)
cms_configure_target(cms_string_buffer_tests)

add_test(NAME cms.string_buffer COMMAND cms_string_buffer_tests)
set_tests_properties(cms.string_buffer PROPERTIES TIMEOUT 30)

add_library(cms_utf8_compile_contract OBJECT
    compile/test_utf8_contract.cpp
)

target_link_libraries(cms_utf8_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_utf8_compile_contract)

add_library(cms_utf8_header_compile OBJECT
    compile/test_utf8_header.cpp
)

target_compile_features(cms_utf8_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_utf8_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_utf8_header_compile)

add_executable(cms_utf8_tests
    unit/test_utf8.cpp
)

target_link_libraries(cms_utf8_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_utf8_tests PRIVATE support)
cms_configure_target(cms_utf8_tests)

add_test(NAME cms.utf8 COMMAND cms_utf8_tests)
set_tests_properties(cms.utf8 PROPERTIES TIMEOUT 30)

add_library(cms_static_string_compile_contract OBJECT
    compile/test_static_string_contract.cpp
)

target_link_libraries(cms_static_string_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_static_string_compile_contract)

add_library(cms_static_string_header_compile OBJECT
    compile/test_static_string_header.cpp
)

target_compile_features(cms_static_string_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_static_string_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_static_string_header_compile)

add_executable(cms_static_string_tests
    unit/test_static_string.cpp
)

target_link_libraries(cms_static_string_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_static_string_tests PRIVATE support)
cms_configure_target(cms_static_string_tests)

add_test(NAME cms.static_string COMMAND cms_static_string_tests)
set_tests_properties(cms.static_string PROPERTIES TIMEOUT 30)
