cmake_minimum_required(VERSION 3.21)
project(memstats LANGUAGES C CXX)

find_package(TBB)
find_package(Threads)

add_library(memstats)
target_sources(memstats PRIVATE memstats.cc)
target_link_libraries(memstats PRIVATE $<TARGET_NAME_IF_EXISTS:TBB::tbb> $<TARGET_NAME_IF_EXISTS:Threads::Threads>)

option(USE_MEMORY_TRACER "Enable memory tracing" OFF)

if(USE_MEMORY_TRACER)
    target_compile_definitions(memstats PRIVATE MEMSTATS_USE_MEMORY_TRACER)

    set(PINTOOL_PATH "" CACHE PATH "")

    if (NOT PINTOOL_PATH)
        message(FATAL_ERROR "For using memstats with memory tracing, the path to the directory of pintool is required.")
    endif()
    execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/memory_tracer/compile.sh ${PINTOOL_PATH})
endif()

include(GNUInstallDirs)
install(FILES memstats.hh
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

target_include_directories(memstats PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src_stacktrace.cxx"
[[
#include <stacktrace>
#include <iostream>
#include <version>
int main(){
    std::cout << std::stacktrace::current();
    return (__cpp_lib_stacktrace < 202011L);
}]])

try_compile(stacktrace ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src_stacktrace.cxx CXX_STANDARD 23)

if (stacktrace)
    message(STATUS "Performing Test stacktrace - Success")
    target_compile_features(memstats PRIVATE cxx_std_23)
    target_compile_definitions(memstats PRIVATE MEMSTAT_HAVE_STACKTRACE)
else()
    message(STATUS "Performing Test stacktrace - Failed")
    target_compile_features(memstats PRIVATE cxx_std_11)
endif()

file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src_atomic_constexpr.cxx"
[[
#include <atomic>
constexpr static std::atomic<bool> atomic_flag = {};
int main(){
    return 0;}
]])

try_compile(atomic_constexpr ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src_atomic_constexpr.cxx CXX_STANDARD 11)

if(atomic_constexpr)
    message(STATUS "Performing Test atomic_constexpr - Success")
    target_compile_definitions(memstats PRIVATE MEMSTAT_ATOMIC_CONSTEXPR)
else()
    message(STATUS "Performing Test atomic_constexpr - Failed")
endif()

target_compile_definitions(memstats PRIVATE $<$<TARGET_EXISTS:TBB::tbb>:MEMSTAT_HAVE_TBB>)
set_target_properties(memstats PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    EXPORT_NAME MemStats)

add_library(MemStats::MemStats ALIAS memstats)

install(TARGETS memstats EXPORT memstats-targets ARCHIVE)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.cmake.in
[[
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)
find_dependency(TBB)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/memstats-targets.cmake")
]])

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_BINARY_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/memstats-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/memstats
)

install(EXPORT memstats-targets
        FILE memstats-targets.cmake
        NAMESPACE MemStats::
        DESTINATION "${INSTALL_LIBDIR}/cmake/memstats"
)

export(EXPORT memstats-targets
       NAMESPACE MemStats::
)

if(memstats_IS_TOP_LEVEL)
    add_executable(example_01 example_01.cc)
    target_link_libraries(example_01 PUBLIC MemStats::MemStats)

    add_executable(example_02 example_02.cc)
    target_link_libraries(example_02 PUBLIC MemStats::MemStats)

    if(TARGET Threads::Threads)
        add_executable(example_03 example_03.cc)
        target_link_libraries(example_03 PUBLIC MemStats::MemStats)
        target_compile_features(example_03 PUBLIC cxx_std_11)
    endif()

    add_executable(example_04 example_04.cc)
    target_link_libraries(example_04 PUBLIC MemStats::MemStats)

    add_executable(example_05 example_05.cc)
    target_link_libraries(example_05 PUBLIC MemStats::MemStats)

    add_executable(example_06 example_06.cc)
    target_link_libraries(example_05 PUBLIC MemStats::MemStats)
endif()