cmake_minimum_required(VERSION 3.1)
project(iscore_addon_pd LANGUAGES CXX)

if(ANDROID)
  set(LIBPD_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpd/libs/android/libpd.a)
else()
  find_library(LIBPD_LIBRARY NAMES libpd pd HINTS 3rdparty/libpd/libs)
endif()
if(NOT LIBPD_LIBRARY)
    message("Please build 3rdparty/libpd (git submodule update --init --recursive && make) ")
    return()
endif()
iscore_common_setup()

# Packages
# Files & main target
set(HDRS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdLayer.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdMetadata.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/ApplicationPlugin.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DocumentPlugin.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowContext.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowClock.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowWindow.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowProcess.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowObjects.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPort.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPd.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditConnection.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/PdInspectorFactory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/PdInspectorWidget.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/DataflowWidget.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Devices/AudioDevice.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Devices/MidiDevice.hpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/AudioNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/AutomationNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/LoopNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/MidiNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/NodeItem.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/PdNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/ScenarioNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/ScriptNode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/View.hpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.hpp"

)

set(SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/PdProcess.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/PdCommandFactory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPort.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditPd.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Commands/EditConnection.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Executor/PdExecutor.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/ApplicationPlugin.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DocumentPlugin.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowContext.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowClock.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowWindow.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/DataflowProcess.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/PdInspectorWidget.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Inspector/DataflowWidget.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Devices/AudioDevice.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/Devices/MidiDevice.cpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/AudioNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/AutomationNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/LoopNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/MidiNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/NodeItem.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/PdNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/ScenarioNode.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/ScriptNode.cpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/Pd/UI/View.cpp"

  "${CMAKE_CURRENT_SOURCE_DIR}/iscore_addon_pd.cpp"
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})

iscore_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC -L/usr/local/lib )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC iscore_plugin_engine ${LIBPD_LIBRARY} nodes portaudio dl)
target_include_directories(${PROJECT_NAME} PUBLIC
  3rdparty/libpd/libpd_wrapper
  3rdparty/libpd/pure-data/src
  )

setup_iscore_plugin(${PROJECT_NAME})


setup_iscore_tests(Tests)
