####
# This sets up the build system for the 'FprimeMlpackReference' project, including
# components and deployments from project.cmake. In addition, it imports the core F Prime components.
####

cmake_minimum_required(VERSION 3.24.2)
project(FprimeMlpackReference C CXX)
# mlpack requires C++17.
set(CMAKE_CXX_STANDARD 17)

###
# F' Core Setup
# This includes all of the F prime core components, and imports the make-system.
###
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
fprime_setup_included_code()


###
# mlpack setup
#
# mlpack provides an auto-downloader in mlpack.cmake via the fetch_mlpack()
# macro.  This provides the ${MLPACK_INCLUDE_DIRS} and ${MLPACK_LIBRARIES}
# variables.
###
include("${CMAKE_CURRENT_LIST_DIR}/CMake/mlpack.cmake")

# If you are cross-compiling, fetch_mlpack() will automatically cross-compile
# OpenBLAS to the target platform.  Set the following options to configure
# OpenBLAS for your device and then call fetch_mlpack().
#
#set(OPENBLAS_TARGET "CORTEXA53")
#set(OPENBLAS_BINARY "64")
#set(OPENBLAS_EXTRA_ARGS "NOFORTRAN=1")
fetch_mlpack(${CMAKE_CROSSCOMPILING})
include_directories(${MLPACK_INCLUDE_DIRS})


# This includes project-wide objects
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ExampleDeployment/")
