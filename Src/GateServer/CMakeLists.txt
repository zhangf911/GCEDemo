if (WIN32)
  set (PROJECT_WINVER "0x0601" CACHE STRING "Windows version maro. Default is 0x0601 - win7, user can reset")
  add_definitions (-D_WIN32_WINNT=${PROJECT_WINVER})
endif ()

include_directories("E:/boost_1_55_0")
include_directories(${PROJECT_SOURCE_DIR}/Library)
include_directories(${PROJECT_SOURCE_DIR}/Common)
include_directories("${PROJECT_SOURCE_DIR}/../libs/json/include")
include_directories("${PROJECT_SOURCE_DIR}/../libs/MySQL/include/x64")
include_directories("${PROJECT_SOURCE_DIR}/../libs/MySQL Connector C++ 1.1.2/include/x64")
include_directories("${PROJECT_SOURCE_DIR}/../libs/redis-cplusplus-client/include")
include_directories("${PROJECT_SOURCE_DIR}/../libs/gce/include")

link_directories(${CMAKE_INSTALL_PREFIX}/lib)
link_directories("E:/boost_1_55_0/stage/VS2013/x64")
link_directories("${PROJECT_SOURCE_DIR}/../libs/gce/lib/x64/")
link_directories("${PROJECT_SOURCE_DIR}/../libs/json/lib/x64/")
link_directories("${PROJECT_SOURCE_DIR}/../libs/MySQL/lib/x64/")
link_directories("${PROJECT_SOURCE_DIR}/../libs/MySQL Connector C++ 1.1.2/lib/x64/")
link_directories("${PROJECT_SOURCE_DIR}/../libs/redis-cplusplus-client/lib/x64/")
# if(CMAKE_BUILD_TYPE MATCHES Debug)
# link_directories("E:/Project/Conqueror/Code/Server/libs/gce/lib/x64/Debug")
# endif()
# if(CMAKE_BUILD_TYPE MATCHES Release)
# link_directories("E:/Project/Conqueror/Code/Server/libs/gce/lib/x64/Release")
# endif()

# aux_source_directory(. GATE_SRC)

file (GLOB_RECURSE GATESERVER_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file (GLOB_RECURSE GATESERVER_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/Bin)

add_executable(GateServer ${GATESERVER_HEADER_FILES} ${GATESERVER_SOURCE_FILES})

target_link_libraries(GateServer Library)
target_link_libraries(GateServer Common)
target_link_libraries(GateServer gce_actor)

install (TARGETS GateServer DESTINATION bin)
install (FILES "${PROJECT_BINARY_DIR}/ConquerorConfig.h"
DESTINATION include)