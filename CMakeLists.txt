cmake_minimum_required(VERSION 3.13)
project(smd5LO VERSION 0.0.1)

# 设置项目包含目录、库目录和可执行文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ROOT 编译选项
# 准备工作：source thisroot.sh
find_package(ROOT)
set(ROOT_CC_FLAG_LIST ${ROOT_CC_FLAGS})
set(ROOT_CXX_FLAG_LIST ${ROOT_CXX_FLAGS})
separate_arguments(ROOT_CC_FLAG_LIST)
separate_arguments(ROOT_CXX_FLAG_LIST)
add_compile_options(${ROOT_CXX_FLAG_LIST})
add_compile_definitions(${ROOT_DEFINITIONS})
include_directories(${ROOT_INCLUDE_DIRS})
link_directories(${ROOT_LIBRARY_DIR})

# ExRootAnalysis 编译选项
if(NOT EXROOTANA_DIR)
    set(EXROOTANA_DIR ${PROJECT_SOURCE_DIR}/resource/MG5_aMC/ExRootAnalysis)
endif()
set(EXROOTANA_INC_DIR ${EXROOTANA_DIR} ${EXROOTANA_DIR}/ExRootAnalysis)
set(EXROOTANA_LIB_DIR ${EXROOTANA_DIR})
include_directories(${EXROOTANA_INC_DIR})
link_directories(${EXROOTANA_LIB_DIR})
add_compile_definitions(EXROOTANA_DIR="${EXROOTANA_DIR}")
add_compile_definitions(EXROOTANA_EXTERNAL_DIR="${EXROOTANA_DIR}/ExRootAnalysis")
add_compile_definitions(LIBEXROOTANA_SO="${EXROOTANA_LIB_DIR}/libExRootAnalysis.so")

# Delphes 编译选项
if(NOT DELPHES_DIR)
    set(DELPHES_DIR ${PROJECT_SOURCE_DIR}/resource/MG5_aMC/Delphes)
endif()
set(DELPHES_INC_DIR ${DELPHES_DIR} ${DELPHES_DIR}/external)
set(DELPHES_LIB_DIR ${DELPHES_DIR})
include_directories(${DELPHES_INC_DIR})
link_directories(${DELPHES_LIB_DIR})
add_compile_definitions(DELPHES_DIR="${DELPHES_DIR}")
add_compile_definitions(DELPHES_EXTERNAL_DIR="${DELPHES_DIR}/external")
add_compile_definitions(LIBDELPHES_SO="${DELPHES_LIB_DIR}/libDelphes.so")

# 优化、调试和通用编译选项
option(ENABLE_SANITIZER "compile and link with sanitizing flags" OFF)
option(ENABLE_GPROF "compile and link with gprof flags" OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELEASE)
endif()
if(CMAKE_BUILD_TYPE STREQUAL DEBUG)
    add_compile_options(-O0 -fno-omit-frame-pointer)
    if(ENABLE_SANITIZER)
        set(SANITIZER_OPTIONS -fno-sanitize-recover=all)
        list(APPEND SANITIZER_OPTIONS -fsanitize=address)
        list(APPEND SANITIZER_OPTIONS -fsanitize=leak)
        list(APPEND SANITIZER_OPTIONS -fsanitize=undefined)
        list(APPEND SANITIZER_OPTIONS -fsanitize=float-divide-by-zero)
        add_compile_options(${SANITIZER_OPTIONS})
        add_link_options(${SANITIZER_OPTIONS})
    endif()
else()
    add_compile_options(-O3)
endif()
if(ENABLE_GPROF)
    set(GPROF_OPTIONS -pg)
    add_compile_options(${GPROF_OPTIONS})
    add_link_options(${GPROF_OPTIONS})
endif()
add_compile_options(-Wall -Wshadow -Wextra)
add_compile_definitions(_GNU_SOURCE)

# 可选：CMSSW 环境
if(CMSSW_RLSE)
    include_directories(/cvmfs/cms.cern.ch/slc7_amd64_gcc700/cms/cmssw/${CMSSW_RLSE}/src)
    link_directories(/cvmfs/cms.cern.ch/slc7_amd64_gcc700/cms/cmssw/${CMSSW_RLSE}/lib/slc7_amd64_gcc700)
    if(NOT FWLITE_LIBRARIES)
        set(FWLITE_LIBRARIES FWCoreFWLite DataFormatsFWLite)
    endif()
    add_compile_definitions(CMSSW_RLSE=${CMSSW_RLSE})
endif()

# 以下为库文件构建

file(GLOB CMS_SOURCES src/cms/*.cpp)
add_library(cms SHARED ${CMS_SOURCES})
target_link_libraries(cms ASImage)

file(GLOB CMS_SOURCES src/smd5/*.cpp)
add_library(smd5 SHARED ${CMS_SOURCES})
target_link_libraries(smd5 ExRootAnalysis Delphes ROOT::Tree)

# 以下为测试程序构建

add_subdirectory(src/test)

# 以下为其它可执行文件构建

add_executable(eg-lhe src/eg-lhe.cpp)
target_link_libraries(eg-lhe smd5)

add_executable(eg-delphes src/eg-delphes.cpp)
target_link_libraries(eg-delphes smd5)

add_executable(plot src/plot.cpp)
target_link_libraries(plot smd5 ROOT::Physics ROOT::Gpad)
