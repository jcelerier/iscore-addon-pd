cmake_minimum_required(VERSION 3.1)
project(iscore_addon_pd LANGUAGES CXX)

if(NOT TARGET iscore_plugin_audio)
    return()
endif()

iscore_common_setup()

# Packages
# Files & main target
set(HDRS 
   "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.hpp"
)

set(SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.cpp"
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})

iscore_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
target_link_libraries(${PROJECT_NAME} PUBLIC iscore_plugin_audio)

setup_iscore_plugin(${PROJECT_NAME})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-error=return-type-c-linkage")
endif()

