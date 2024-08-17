file(GLOB_RECURSE SOURCE_FILE "*.cpp")

add_library(${PROJECT_SOURCE_TARGET} ${SOURCE_FILE})

target_include_directories(${PROJECT_SOURCE_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/analysis
)
