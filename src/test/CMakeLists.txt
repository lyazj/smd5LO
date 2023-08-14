cmake_minimum_required(VERSION 3.13)
project(smd5LO-test VERSION 0.0.1)

# 设置项目包含目录、库目录和可执行文件目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/test)

# 以下为其它可执行文件构建

add_executable(test-list-run test-list-run.cpp)
target_link_libraries(test-list-run smd5)
