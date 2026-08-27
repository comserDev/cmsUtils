cmake_minimum_required(VERSION 3.20)

project(cmsUtilsPackageConsumer LANGUAGES CXX)

find_package(cmsUtils CONFIG REQUIRED)

add_executable(cms_utils_package_consumer main.cpp)
target_link_libraries(cms_utils_package_consumer PRIVATE cms::utils)
