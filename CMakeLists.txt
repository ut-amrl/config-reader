cmake_minimum_required(VERSION 3.10)
project(config_reader)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Lua REQUIRED)

set(CONFIG_READER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(NOT EXISTS "${CONFIG_READER_INCLUDE_DIR}/config_reader/config_reader.h")
  message(FATAL_ERROR
    "Missing config-reader headers at ${CONFIG_READER_INCLUDE_DIR}.")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CONFIG_READER_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  ${EIGEN3_INCLUDE_DIRS}
  ${LUA_INCLUDE_DIR}
)

if(TARGET Lua::Lua)
  target_link_libraries(${PROJECT_NAME} INTERFACE Lua::Lua)
else()
  target_link_libraries(${PROJECT_NAME} INTERFACE ${LUA_LIBRARIES})
endif()

install(
  DIRECTORY "${CONFIG_READER_INCLUDE_DIR}/"
  DESTINATION include
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(Eigen3 Lua)
ament_export_include_directories(include)

ament_package()
