cmake_minimum_required(VERSION 3.1)
project(iscore_addon_pd LANGUAGES CXX)

iscore_common_setup()

# Packages
# Files & main target
set(HDRS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdMetadata.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.hpp"
)

set(SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.cpp"
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})

iscore_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
find_library(LIBPD_LIBRARY NAMES libpd pd HINTS 3rdparty/libpd/libs)
target_link_libraries(${PROJECT_NAME} PUBLIC iscore_plugin_engine ${LIBPD_LIBRARY})
target_include_directories(${PROJECT_NAME} PRIVATE
  3rdparty/libpd/libpd_wrapper
  3rdparty/libpd/pure-data/src
  )

setup_iscore_plugin(${PROJECT_NAME})

