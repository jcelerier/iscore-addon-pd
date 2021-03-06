project(score_addon_pd LANGUAGES CXX)

set(PD_MULTI ON CACHE INTERNAL "")
add_subdirectory(3rdparty/libpd)

if(NOT TARGET libpd)
    message("Problem occuring when building libpd")
    return()
endif()
score_common_setup()

# Packages
# Files & main target
set(HDRS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdLayer.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdMetadata.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdLibrary.hpp"
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
target_include_directories(${PROJECT_NAME}
  PUBLIC
  3rdparty/libpd/libpd_wrapper
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    score_plugin_engine
    score_plugin_scenario
    libpd_static)

setup_score_plugin(${PROJECT_NAME})


# setup_score_tests(Tests)
