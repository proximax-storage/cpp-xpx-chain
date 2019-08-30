cmake_minimum_required(VERSION 3.2)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(
	CMAKE_TOOLCHAIN_FILE
	"${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake"
	CACHE
	FILEPATH
	"Default toolchain"
)

include("cmake/Hunter/init.cmake")
HunterGate(
		URL "https://github.com/cpp-pm/hunter/archive/v0.23.213.tar.gz"
		SHA1 "f7dece30a8fd9e82d9342a55df8bf22c21a2760c"
		FILEPATH ${CMAKE_SOURCE_DIR}/cmake/Hunter/config.cmake
)

project(catapult_server)
include(CMakeGlobalSettings.cmake)
option(ENABLE_CODE_COVERAGE "Enable code coverage" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS          ON)
set(CMAKE_POSITION_INDEPENDENT_CODE        ON)
set_property(GLOBAL PROPERTY USE_FOLDERS   ON)

include(cmake/dependencies.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
message(STATUS "CATAPULT_VERSION_DESCRIPTION: ${CATAPULT_VERSION_DESCRIPTION}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(CORE_CATAPULT_LIBS catapult.io catapult.ionet catapult.model catapult.thread catapult.utils)

# used to add rocksdb dependencies to a target
function(catapult_add_rocksdb_dependencies TARGET_NAME)
	target_link_libraries(${TARGET_NAME} RocksDB::rocksdb)
endfunction()

### add source directories
add_subdirectory(external)

# run catapult tests first
add_subdirectory(src)
add_subdirectory(tests)

# run other tests second
add_subdirectory(sdk)
add_subdirectory(plugins)
add_subdirectory(extensions)

add_subdirectory(resources)

if (EXISTS "${CMAKE_SOURCE_DIR}/internal/CMakeLists.txt")
	message("--- adding internal directory ---")
	add_subdirectory(internal)
endif()

add_subdirectory(tools)

# Help programs
add_subdirectory(spammer)
