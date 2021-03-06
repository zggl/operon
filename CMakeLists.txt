cmake_minimum_required(VERSION 3.11)

include(FetchContent)
include(FeatureSummary)

project(
    Operon
    VERSION 0.1
    DESCRIPTION "Genetic Programming"
    LANGUAGES CXX
)

find_package(Git)
find_package(cxxopts)
find_package(fmt)
find_package(Eigen3)
find_package(Ceres QUIET)
find_package(TBB)
find_package(Python3 COMPONENTS Development)
find_package(pybind11)
find_package(Catch2)

# operon library
set_package_properties(Git     PROPERTIES TYPE REQUIRED)
set_package_properties(cxxopts PROPERTIES TYPE REQUIRED)
set_package_properties(fmt     PROPERTIES TYPE REQUIRED)
set_package_properties(Eigen3  PROPERTIES TYPE REQUIRED)
set_package_properties(Ceres   PROPERTIES TYPE REQUIRED)
set_package_properties(Tbb     PROPERTIES TYPE REQUIRED)

# unit tests
set_package_properties(catch2 PROPERTIES TYPE OPTIONAL)

# python binding
set_package_properties(Python3 PROPERTIES TYPE OPTIONAL)
set_package_properties(pybind11 PROPERTIES TYPE OPTIONAL)

FetchContent_Declare(
    gsl
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG master
    DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/thirdparty/GSL
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/GSL
)

FetchContent_Declare(
    csv
    DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/thirdparty/CSV/include
    URL https://raw.githubusercontent.com/vincentlaucsb/csv-parser/master/single_include/csv.hpp
    DOWNLOAD_NO_EXTRACT 1
)

FetchContent_Declare(
    xxhash
    DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/thirdparty/xxhash
    URL https://raw.githubusercontent.com/RedSpah/xxhash_cpp/master/include/xxhash.hpp
    DOWNLOAD_NO_EXTRACT 1
)

FetchContent_MakeAvailable(xxhash gsl csv)

set(THIRDPARTY_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/thirdparty/GSL/include
    ${PROJECT_SOURCE_DIR}/thirdparty/CSV/include
    ${PROJECT_SOURCE_DIR}/thirdparty/
)

# compile and link-time options
set(BUILD_TESTS_DESCRIPTION          "Build unit tests.")
set(BUILD_PYBIND_DESCRIPTION         "Build python bindings.")
set(OPENLIBM_DESCRIPTION             "Link against Julia's openlibm, a high performance mathematical library.")
set(JEMALLOC_DESCRIPTION             "Link against jemalloc, a general purpose malloc(3) implementation that emphasizes fragmentation avoidance and scalable concurrency support.")
set(TCMALLOC_DESCRIPTION             "Link against tcmalloc (thread-caching malloc), a malloc(3) implementation that reduces lock contention for multi-threaded programs.")
set(USE_SINGLE_PRECISION_DESCRIPTION "Perform model evaluation using floats (single precision) instead of doubles. Great for reducing runtime, might not be appropriate for all purposes.")

# option descriptions
option(BUILD_TESTS          ${BUILD_TESTS_DESCRIPTION}          OFF)
option(BUILD_PYBIND         ${BUILD_PYBIND_DESCRIPTION}         OFF)
option(USE_OPENLIBM         ${OPENLIBM_DESCRIPTION}             OFF)
option(USE_JEMALLOC         ${JEMALLOC_DESCRIPTION}             OFF)
option(USE_TCMALLOC         ${TCMALLOC_DESCRIPTION}             OFF)
option(USE_SINGLE_PRECISION ${USE_SINGLE_PRECISION_DESCRIPTION} OFF)

add_feature_info(BUILD_TESTS          BUILD_TESTS          ${BUILD_TESTS_DESCRIPTION})
add_feature_info(BUILD_PYBIND         BUILD_PYBIND         ${BUILD_PYBIND_DESCRIPTION})
add_feature_info(USE_OPENLIBM         USE_OPENLIBM         ${OPENLIBM_DESCRIPTION})
add_feature_info(USE_JEMALLOC         USE_JEMALLOC         ${JEMALLOC_DESCRIPTION})
add_feature_info(USE_TCMALLOC         USE_TCMALLOC         ${TCMALLOC_DESCRIPTION})
add_feature_info(USE_SINGLE_PRECISION USE_SINGLE_PRECISION ${USE_SINGLE_PRECISION_DESCRIPTION})

if(USE_JEMALLOC AND USE_TCMALLOC)
    message(FATAL_ERROR "Options USE_JEMALLOC and USE_TCMALLOC are mutually exclusive. Please specify only one.")
endif()

if(USE_OPENLIBM)
    find_library(OPENLIBM openlibm)
    if(NOT OPENLIBM)
        message(WARNING "Option USE_OPENLIBM was specified, but openlibm could not be found.")
        set(OPENLIBM "")
    else ()
        message(STATUS "Option USE_OPENLIBM was specified, found openlibm at ${OPENLIBM}.")
    endif()
endif()

if(USE_JEMALLOC)
    find_library(JEMALLOC jemalloc)
    if(NOT JEMALLOC)
        message(WARNING "Option USE_JEMALLOC was specified, but jemalloc could not be found.")
        set(JEMALLOC "")
    else()
        message(STATUS "Option USE_JEMALLOC was specified, found jemalloc at ${JEMALLOC}.")
    endif()
endif()

if(USE_TCMALLOC)
    find_library(TCMALLOC tcmalloc)
    if(NOT TCMALLOC)
        message(WARNING "Option USE_TCMALLOC was specified, but tcmalloc could not be found.")
        set(TCMALLOC "")
    else()
        message(STATUS "Option USE_TCMALLOC was specified, found tcmalloc at ${TCMALLOC}.")
    endif()
endif()

if(USE_SINGLE_PRECISION)
    message(STATUS "Option USE_SINGLE_PRECISION was specified, single-precision model evaluation will be used.")
endif()

# print a status of what we found
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:" QUIET_ON_EMPTY)
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:" QUIET_ON_EMPTY)

add_library(
    operon
    SHARED
    src/core/metrics.cpp
    src/core/tree.cpp
    src/core/problem.cpp
    src/core/dataset.cpp
    src/operators/crossover.cpp
    src/operators/mutation.cpp
    src/operators/creator/balanced.cpp
    src/operators/creator/ptc2.cpp
    src/stat/meanvariance.cpp
    src/stat/pearson.cpp
)
target_compile_features(operon PRIVATE cxx_std_17)
target_link_libraries(operon PRIVATE fmt::fmt ${OPENLIBM} ${JEMALLOC} ${TCMALLOC} ${CERES_LIBRARIES} TBB::tbb)
target_include_directories(
    operon
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/operon
    ${THIRDPARTY_INCLUDE_DIRS}
    ${CERES_INCLUDE_DIRS}
)
# necessary to prevent -isystem introduced by intel-tbb
# set_target_properties(operon PROPERTIES NO_SYSTEM_FROM_IMPORTED TRUE)
target_compile_definitions(operon PRIVATE "$<$<BOOL:${USE_SINGLE_PRECISION}>:USE_SINGLE_PRECISION>")

#binary for GP algorithm cli version
add_executable(
    operon-gp
    src/cli/operon_gp.cpp
)
target_compile_features(operon-gp PRIVATE cxx_std_17)
target_link_libraries(operon-gp PRIVATE operon fmt::fmt cxxopts::cxxopts ${OPENLIBM} ${JEMALLOC} ${TCMALLOC} ${CERES_LIBRARIES} TBB::tbb)
target_include_directories(
    operon-gp
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/operon
    ${THIRDPARTY_INCLUDE_DIRS}
    ${CERES_INCLUDE_DIRS}
)
#set_target_properties(operon-gp PROPERTIES NO_SYSTEM_FROM_IMPORTED TRUE)
target_compile_definitions(operon-gp PRIVATE "$<$<BOOL:${USE_SINGLE_PRECISION}>:USE_SINGLE_PRECISION>")

add_executable(
    operon-example-gp
    examples/gp.cpp
    )
target_compile_features(operon-example-gp PRIVATE cxx_std_17)
target_link_libraries(operon-example-gp PRIVATE operon fmt::fmt cxxopts::cxxopts ${OPENLIBM} ${JEMALLOC} ${TCMALLOC} ${CERES_LIBRARIES} TBB::tbb)
target_include_directories(
    operon-example-gp
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/operon
    ${THIRDPARTY_INCLUDE_DIRS}
    ${CERES_INCLUDE_DIRS}
)
#set_target_properties(operon-example-gp PROPERTIES NO_SYSTEM_FROM_IMPORTED TRUE)
target_compile_definitions(operon-example-gp PRIVATE "$<$<BOOL:${USE_SINGLE_PRECISION}>:USE_SINGLE_PRECISION>")

if(MSVC)
    target_compile_options(operon PRIVATE /W4 "$<$<CONFIG:Release>:/O2;/std:c++latest>")
    target_compile_options(operon-gp PRIVATE /W4 "$<$<CONFIG:Release>:/O2;/std:c++latest>")
    target_compile_options(operon-test PRIVATE /W4 "$<$<CONFIG:Release>:/O2;/std:c++latest>")
    # ugly hack to set the include dir correctly
    find_path(FMT_INCLUDE_DIR fmt/core.h)
    include_directories(${FMT_INCLUDE_DIR})
else()
    set(MYFLAGS -Wall -Wextra -Wno-unknown-pragmas -Wno-deprecated -Wno-deprecated-copy)
    target_compile_options(operon PRIVATE ${MYFLAGS} "$<$<CONFIG:Debug>:-g;--coverage>$<$<CONFIG:Release>:-O3;-g;-march=native>")
    target_link_libraries(operon PRIVATE "$<$<CONFIG:Debug>:gcov>")
    target_compile_options(operon-gp PRIVATE ${MYFLAGS} "$<$<CONFIG:Debug>:-g;--coverage>$<$<CONFIG:Release>:-O3;-g;-march=native>")
    target_link_libraries(operon-gp PRIVATE "$<$<CONFIG:Debug>:gcov>")
    target_compile_options(operon-example-gp PRIVATE ${MYFLAGS} "$<$<CONFIG:Debug>:-g;--coverage>$<$<CONFIG:Release>:-O3;-g;-march=native>")
    target_link_libraries(operon-example-gp PRIVATE "$<$<CONFIG:Debug>:gcov>")
endif(MSVC)

if(Python3_FOUND AND pybind11_FOUND AND BUILD_PYBIND)
    add_library(
        pyoperon
        SHARED
        python/operon.cpp
        )
    target_compile_features(pyoperon PRIVATE cxx_std_17)
    target_link_libraries(pyoperon PRIVATE operon ${OPENLIBM} ${JEMALLOC} ${TCMALLOC} ${TCMALLOC} TBB::tbb) # figure out if tbb is necessary for linkage here
    target_compile_options(pyoperon PRIVATE ${MYFLAGS} "$<$<CONFIG:Debug>:-g;-fPIC;--coverage>$<$<CONFIG:Release>:-O3;-g;-fPIC;-march=native>")
    target_include_directories(
        pyoperon
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include/operon
        ${PROJECT_SOURCE_DIR}/python
        ${THIRDPARTY_INCLUDE_DIRS}
        ${Python3_INCLUDE_DIRS}
        ${CERES_INCLUDE_DIRS}
        )
endif()

if(Catch2_FOUND AND BUILD_TESTS)
    add_executable(
        operon-test
        test/test.cpp
        test/performance/evaluation.cpp
        test/performance/initialization.cpp
        test/performance/hashing.cpp
        test/performance/distance.cpp
        test/implementation/evaluation.cpp
        test/implementation/details.cpp
        test/implementation/hashing.cpp
        test/implementation/initialization.cpp
        test/implementation/selection.cpp
        #        test/codegen/irbuilder.cpp
        )
    target_compile_features(operon-test PRIVATE cxx_std_17)
    target_link_libraries(operon-test PRIVATE operon fmt::fmt Catch2::Catch2 ${OPENLIBM} ${JEMALLOC} ${TCMALLOC} ${CERES_LIBRARIES} TBB::tbb)
    target_include_directories(
        operon-test
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include/operon
        ${THIRDPARTY_INCLUDE_DIRS}
        ${CERES_INCLUDE_DIRS}
        )
    target_compile_definitions(operon-test PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING)
    #set_target_properties(operon-test PROPERTIES NO_SYSTEM_FROM_IMPORTED TRUE)
    target_compile_definitions(operon-test PRIVATE "$<$<BOOL:${USE_SINGLE_PRECISION}>:USE_SINGLE_PRECISION>")
    target_compile_options(operon-test PRIVATE ${MYFLAGS} "$<$<CONFIG:Debug>:-g;--coverage>$<$<CONFIG:Release>:-O3;-g;-march=native>")
    target_link_libraries(operon-test PRIVATE "$<$<CONFIG:Debug>:gcov>")

    include(CTest)
    include(Catch)
    catch_discover_tests(operon-test)
endif()

