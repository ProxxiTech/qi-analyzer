cmake_minimum_required (VERSION 3.13)

project(QiAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/QiAnalyzer.cpp
src/QiAnalyzer.h
src/QiAnalyzerResults.cpp
src/QiAnalyzerResults.h
src/QiAnalyzerSettings.cpp
src/QiAnalyzerSettings.h
src/QiSimulationDataGenerator.cpp
src/QiSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
