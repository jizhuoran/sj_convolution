cmake_minimum_required(VERSION 2.8.7)


file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp)




message(INFO ${srcs})

#set(CMAKE_CXX_FLAGS "-g")

add_definitions(-DUSE_OPEN_BLAS)
include(./cmake/Modules/FindOpenBLAS.cmake)
include_directories(${OpenBLAS_INCLUDE_DIR})
list(APPEND STEAM_LINKER_LIBS ${OpenBLAS_LIB}) 

include(./cmake/Modules/FindOpenCL.cmake)
include_directories(${OPENCL_INCLUDE_DIRS})
list(APPEND STEAM_LINKER_LIBS ${OPENCL_LIBRARIES})  

include(./cmake/Modules/FindCLBlas.cmake)
include_directories(${CLBLAS_INCLUDE_DIRS})
list(APPEND STEAM_LINKER_LIBS ${CLBLAS_LIBRARIES})


add_executable(main ${srcs} main.cpp)
target_link_libraries(main ${STEAM_LINKER_LIBS})
