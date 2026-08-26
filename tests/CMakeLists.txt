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

add_library(cms_string_ops_compile_contract OBJECT
    compile/test_string_ops_contract.cpp
)

target_link_libraries(cms_string_ops_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_string_ops_compile_contract)

add_library(cms_string_ops_header_compile OBJECT
    compile/test_string_ops_header.cpp
)

target_compile_features(cms_string_ops_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_string_ops_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_string_ops_header_compile)

add_executable(cms_string_ops_tests
    unit/test_string_ops.cpp
)

target_link_libraries(cms_string_ops_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_string_ops_tests PRIVATE support)
cms_configure_target(cms_string_ops_tests)

add_test(NAME cms.string_ops COMMAND cms_string_ops_tests)
set_tests_properties(cms.string_ops PROPERTIES TIMEOUT 30)

add_library(cms_format_compile_contract OBJECT
    compile/test_format_contract.cpp
)

target_link_libraries(cms_format_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_format_compile_contract)

add_library(cms_format_header_compile OBJECT
    compile/test_format_header.cpp
)

target_compile_features(cms_format_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_format_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_format_header_compile)

add_executable(cms_format_tests
    unit/test_format.cpp
)

target_link_libraries(cms_format_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_format_tests PRIVATE support)
cms_configure_target(cms_format_tests)

add_test(NAME cms.format COMMAND cms_format_tests)
set_tests_properties(cms.format PROPERTIES TIMEOUT 30)

add_library(cms_parse_compile_contract OBJECT
    compile/test_parse_contract.cpp
)

target_link_libraries(cms_parse_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_parse_compile_contract)

add_library(cms_parse_header_compile OBJECT
    compile/test_parse_header.cpp
)

target_compile_features(cms_parse_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_parse_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_parse_header_compile)

add_executable(cms_parse_tests
    unit/test_parse.cpp
)

target_link_libraries(cms_parse_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_parse_tests PRIVATE support)
cms_configure_target(cms_parse_tests)

add_test(NAME cms.parse COMMAND cms_parse_tests)
set_tests_properties(cms.parse PROPERTIES TIMEOUT 30)

add_library(cms_static_queue_compile_contract OBJECT
    compile/test_static_queue_contract.cpp
)

target_link_libraries(cms_static_queue_compile_contract PRIVATE
    cms_embedded_utils
)
cms_configure_target(cms_static_queue_compile_contract)

add_library(cms_static_queue_header_compile OBJECT
    compile/test_static_queue_header.cpp
)

target_compile_features(cms_static_queue_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_static_queue_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_static_queue_header_compile)

add_executable(cms_static_queue_tests
    unit/test_static_queue.cpp
)

target_link_libraries(cms_static_queue_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_static_queue_tests PRIVATE support)
cms_configure_target(cms_static_queue_tests)

add_test(NAME cms.static_queue COMMAND cms_static_queue_tests)
set_tests_properties(cms.static_queue PROPERTIES TIMEOUT 30)

add_library(cms_sync_compile_contract OBJECT
    compile/test_sync_contract.cpp
)

target_link_libraries(cms_sync_compile_contract PRIVATE cms_embedded_utils)
cms_configure_target(cms_sync_compile_contract)

add_library(cms_synchronized_queue_compile_contract OBJECT
    compile/test_synchronized_queue_contract.cpp
)

target_link_libraries(cms_synchronized_queue_compile_contract PRIVATE
    cms_embedded_utils
)
cms_configure_target(cms_synchronized_queue_compile_contract)

add_library(cms_lock_guard_header_compile OBJECT
    compile/test_lock_guard_header.cpp
)

target_compile_features(cms_lock_guard_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_lock_guard_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_lock_guard_header_compile)

add_library(cms_null_mutex_header_compile OBJECT
    compile/test_null_mutex_header.cpp
)

target_compile_features(cms_null_mutex_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_null_mutex_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_null_mutex_header_compile)

add_library(cms_mutex_ref_header_compile OBJECT
    compile/test_mutex_ref_header.cpp
)

target_compile_features(cms_mutex_ref_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_mutex_ref_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_mutex_ref_header_compile)

add_library(cms_synchronized_queue_header_compile OBJECT
    compile/test_synchronized_queue_header.cpp
)

target_compile_features(cms_synchronized_queue_header_compile PRIVATE cxx_std_17)
target_include_directories(cms_synchronized_queue_header_compile PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
cms_configure_target(cms_synchronized_queue_header_compile)

add_executable(cms_synchronized_queue_tests
    unit/test_synchronized_queue.cpp
)

target_link_libraries(cms_synchronized_queue_tests PRIVATE cms_embedded_utils)
target_include_directories(cms_synchronized_queue_tests PRIVATE support)
cms_configure_target(cms_synchronized_queue_tests)

add_test(NAME cms.synchronized_queue COMMAND cms_synchronized_queue_tests)

add_library(cms_log_compile_contract OBJECT
    compile/test_log_contract.cpp
)

target_link_libraries(cms_log_compile_contract
    PRIVATE
        cms_embedded_utils
)

target_compile_features(cms_log_compile_contract PRIVATE cxx_std_17)
cms_configure_target(cms_log_compile_contract)

add_library(cms_log_level_header_compile OBJECT
    compile/test_log_level_header.cpp
)

target_include_directories(cms_log_level_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_level_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_level_header_compile)

add_library(cms_log_clock_header_compile OBJECT
    compile/test_log_clock_header.cpp
)

target_include_directories(cms_log_clock_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_clock_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_clock_header_compile)

add_library(cms_log_sink_header_compile OBJECT
    compile/test_log_sink_header.cpp
)

target_include_directories(cms_log_sink_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_sink_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_sink_header_compile)

add_library(cms_log_record_header_compile OBJECT
    compile/test_log_record_header.cpp
)

target_include_directories(cms_log_record_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_record_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_record_header_compile)

add_library(cms_log_formatter_header_compile OBJECT
    compile/test_log_formatter_header.cpp
)

target_include_directories(cms_log_formatter_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_formatter_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_formatter_header_compile)

add_library(cms_log_async_logger_header_compile OBJECT
    compile/test_log_async_logger_header.cpp
)

target_include_directories(cms_log_async_logger_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_log_async_logger_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_log_async_logger_header_compile)

add_executable(cms_log_record_tests
    unit/test_log_record.cpp
)

target_link_libraries(cms_log_record_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_log_record_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_log_record_tests PRIVATE cxx_std_17)
cms_configure_target(cms_log_record_tests)
add_test(NAME cms.log_record COMMAND cms_log_record_tests)

add_executable(cms_log_formatter_tests
    unit/test_log_formatter.cpp
)

target_link_libraries(cms_log_formatter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_log_formatter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_log_formatter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_log_formatter_tests)
add_test(NAME cms.log_formatter COMMAND cms_log_formatter_tests)

add_executable(cms_async_logger_tests
    unit/test_async_logger.cpp
)

target_link_libraries(cms_async_logger_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_async_logger_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_tests)
add_test(NAME cms.async_logger COMMAND cms_async_logger_tests)
set_tests_properties(cms.synchronized_queue PROPERTIES TIMEOUT 30)

add_library(cms_platform_compile_contract OBJECT
    compile/test_platform_contract.cpp
)

target_link_libraries(cms_platform_compile_contract
    PRIVATE
        cms_embedded_utils
)

target_compile_features(cms_platform_compile_contract PRIVATE cxx_std_17)
cms_configure_target(cms_platform_compile_contract)

add_library(cms_embedded_platform_compile_contract OBJECT
    compile/test_embedded_platform_contract.cpp
)

target_include_directories(cms_embedded_platform_compile_contract
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/arduino
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/freertos
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_embedded_platform_compile_contract PRIVATE cxx_std_17)
cms_configure_target(cms_embedded_platform_compile_contract)

add_library(cms_ansi_formatter_header_compile OBJECT
    compile/test_ansi_formatter_header.cpp
)

target_include_directories(cms_ansi_formatter_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_ansi_formatter_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_ansi_formatter_header_compile)

add_library(cms_std_mutex_header_compile OBJECT
    compile/test_std_mutex_header.cpp
)

target_include_directories(cms_std_mutex_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_std_mutex_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_std_mutex_header_compile)

add_library(cms_steady_clock_header_compile OBJECT
    compile/test_steady_clock_header.cpp
)

target_include_directories(cms_steady_clock_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_steady_clock_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_steady_clock_header_compile)

add_library(cms_stdout_sink_header_compile OBJECT
    compile/test_stdout_sink_header.cpp
)

target_include_directories(cms_stdout_sink_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_stdout_sink_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_stdout_sink_header_compile)

add_library(cms_arduino_serial_sink_header_compile OBJECT
    compile/test_arduino_serial_sink_header.cpp
)

target_include_directories(cms_arduino_serial_sink_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_arduino_serial_sink_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_arduino_serial_sink_header_compile)

add_library(cms_arduino_millis_clock_header_compile OBJECT
    compile/test_arduino_millis_clock_header.cpp
)

target_include_directories(cms_arduino_millis_clock_header_compile
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/arduino
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_arduino_millis_clock_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_arduino_millis_clock_header_compile)

add_library(cms_freertos_static_mutex_header_compile OBJECT
    compile/test_freertos_static_mutex_header.cpp
)

target_include_directories(cms_freertos_static_mutex_header_compile
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/freertos
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_freertos_static_mutex_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_freertos_static_mutex_header_compile)

add_executable(cms_ansi_formatter_tests
    unit/test_ansi_formatter.cpp
)

target_link_libraries(cms_ansi_formatter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_ansi_formatter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_ansi_formatter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_ansi_formatter_tests)
add_test(NAME cms.ansi_formatter COMMAND cms_ansi_formatter_tests)

add_executable(cms_platform_adapter_tests
    unit/test_platform_adapters.cpp
)

target_link_libraries(cms_platform_adapter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_platform_adapter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_platform_adapter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_platform_adapter_tests)
add_test(NAME cms.platform_adapters COMMAND cms_platform_adapter_tests)

add_executable(cms_embedded_adapter_tests
    unit/test_embedded_adapters.cpp
)

target_link_libraries(cms_embedded_adapter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_embedded_adapter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/arduino
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/freertos
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_embedded_adapter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_embedded_adapter_tests)
add_test(NAME cms.embedded_adapters COMMAND cms_embedded_adapter_tests)

add_executable(cms_async_logger_ansi_tests
    unit/test_async_logger_ansi.cpp
)

target_link_libraries(cms_async_logger_ansi_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_ansi_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_async_logger_ansi_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_ansi_tests)
add_test(NAME cms.async_logger_ansi COMMAND cms_async_logger_ansi_tests)

add_library(cms_runtime_ansi_formatter_header_compile OBJECT
    compile/test_runtime_ansi_formatter_header.cpp
)

target_include_directories(cms_runtime_ansi_formatter_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_runtime_ansi_formatter_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_runtime_ansi_formatter_header_compile)

add_executable(cms_runtime_ansi_formatter_tests
    unit/test_runtime_ansi_formatter.cpp
)

target_link_libraries(cms_runtime_ansi_formatter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_runtime_ansi_formatter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_runtime_ansi_formatter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_runtime_ansi_formatter_tests)
add_test(
    NAME cms.runtime_ansi_formatter
    COMMAND cms_runtime_ansi_formatter_tests)

add_executable(cms_async_logger_runtime_ansi_tests
    unit/test_async_logger_runtime_ansi.cpp
)

target_link_libraries(cms_async_logger_runtime_ansi_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_runtime_ansi_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(
    cms_async_logger_runtime_ansi_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_runtime_ansi_tests)
add_test(
    NAME cms.async_logger_runtime_ansi
    COMMAND cms_async_logger_runtime_ansi_tests)

add_library(cms_level_filter_header_compile OBJECT
    compile/test_level_filter_header.cpp
)

target_include_directories(cms_level_filter_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_level_filter_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_level_filter_header_compile)

add_executable(cms_level_filter_tests
    unit/test_level_filter.cpp
)

target_link_libraries(cms_level_filter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_level_filter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_level_filter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_level_filter_tests)
add_test(NAME cms.level_filter COMMAND cms_level_filter_tests)

add_executable(cms_async_logger_runtime_level_tests
    unit/test_async_logger_runtime_level.cpp
)

target_link_libraries(cms_async_logger_runtime_level_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_runtime_level_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(
    cms_async_logger_runtime_level_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_runtime_level_tests)
add_test(
    NAME cms.async_logger_runtime_level
    COMMAND cms_async_logger_runtime_level_tests)

add_executable(cms_async_logger_runtime_features_tests
    unit/test_async_logger_runtime_features.cpp
)

target_link_libraries(cms_async_logger_runtime_features_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_runtime_features_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(
    cms_async_logger_runtime_features_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_runtime_features_tests)
add_test(
    NAME cms.async_logger_runtime_features
    COMMAND cms_async_logger_runtime_features_tests)

add_library(cms_styled_ansi_formatter_header_compile OBJECT
    compile/test_styled_ansi_formatter_header.cpp
)

target_include_directories(cms_styled_ansi_formatter_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    cms_styled_ansi_formatter_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_styled_ansi_formatter_header_compile)

add_executable(cms_styled_ansi_formatter_tests
    unit/test_styled_ansi_formatter.cpp
)

target_link_libraries(cms_styled_ansi_formatter_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_styled_ansi_formatter_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_styled_ansi_formatter_tests PRIVATE cxx_std_17)
cms_configure_target(cms_styled_ansi_formatter_tests)
add_test(
    NAME cms.styled_ansi_formatter
    COMMAND cms_styled_ansi_formatter_tests)

add_executable(cms_async_logger_styled_tests
    unit/test_async_logger_styled.cpp
)

target_link_libraries(cms_async_logger_styled_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_async_logger_styled_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_async_logger_styled_tests PRIVATE cxx_std_17)
cms_configure_target(cms_async_logger_styled_tests)
add_test(
    NAME cms.async_logger_styled
    COMMAND cms_async_logger_styled_tests)

add_library(cms_printf_log_header_compile OBJECT
    compile/test_printf_log_header.cpp
)

target_include_directories(cms_printf_log_header_compile
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cms_printf_log_header_compile PRIVATE cxx_std_17)
cms_configure_target(cms_printf_log_header_compile)

add_executable(cms_printf_log_tests
    unit/test_printf_log.cpp
)

target_link_libraries(cms_printf_log_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_printf_log_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_printf_log_tests PRIVATE cxx_std_17)
cms_configure_target(cms_printf_log_tests)
add_test(NAME cms.printf_log COMMAND cms_printf_log_tests)

add_executable(cms_printf_log_integration_tests
    unit/test_printf_log_integration.cpp
)

target_link_libraries(cms_printf_log_integration_tests
    PRIVATE
        cms_embedded_utils
)

target_include_directories(cms_printf_log_integration_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_features(cms_printf_log_integration_tests PRIVATE cxx_std_17)
cms_configure_target(cms_printf_log_integration_tests)
add_test(
    NAME cms.printf_log_integration
    COMMAND cms_printf_log_integration_tests)
