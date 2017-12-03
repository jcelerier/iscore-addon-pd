cmake_minimum_required(VERSION 3.1)
project(score_addon_pd LANGUAGES CXX)

if(ANDROID)
  set(LIBPD_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpd/libs/android/libpd.a)
else()
  find_library(LIBPD_LIBRARY NAMES libpd pd HINTS 3rdparty/libpd/libs)
endif()
if(NOT LIBPD_LIBRARY)
    message("Please build 3rdparty/libpd (git submodule update --init --recursive && make) ")
    return()
endif()
score_common_setup()

# Packages
# Files & main target
set(HDRS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdLayer.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdMetadata.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPd.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/PdInspectorWidget.hpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/score_addon_pd.hpp"

)

set(SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPd.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/PdInspectorWidget.cpp"


  "${CMAKE_CURRENT_SOURCE_DIR}/score_addon_pd.cpp"
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})

score_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC -L/usr/local/lib )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC score_plugin_engine score_plugin_scenario ${LIBPD_LIBRARY} dl)
target_include_directories(${PROJECT_NAME} PUBLIC
  3rdparty/libpd/libpd_wrapper
  3rdparty/libpd/pure-data/src
  ${OSSIA_3RDPARTY_FOLDER}/exprtk
  )

setup_score_plugin(${PROJECT_NAME})


setup_score_tests(Tests)
