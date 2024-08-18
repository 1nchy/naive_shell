cmake_minimum_required(VERSION 3.26)
project(naive_shell)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE "Release")

set(PROJECT_SOURCE_TARGET ${PROJECT_NAME}_source_target)

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory(third)
if (THIRD_INCLUDE_LIST)
    include_directories(${THIRD_INCLUDE_LIST})
endif()
if (THIRD_LIB_DIR_LIST)
    link_directories(${THIRD_LIB_DIR_LIST})
endif()
if (THIRD_LIB_LIST)
    link_libraries(${THIRD_LIB_LIST})
    # target_link_libraries(${PROJECT_NAME} 
    #     PUBLIC ${THIRD_LIB_LIST}
    # )
endif()

add_subdirectory(src)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} 
    PUBLIC ${PROJECT_SOURCE_TARGET}
)