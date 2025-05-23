cmake_minimum_required(VERSION 3.2)

include_directories(.)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)

function(catapult_define_extension EXTENSION_NAME)
	add_subdirectory(src)
	if(DO_NOT_SKIP_BUILD_TESTS)
		add_subdirectory(tests)
	endif()


	catapult_shared_library_target(extension.${EXTENSION_NAME})
	target_link_libraries(extension.${EXTENSION_NAME} catapult.${EXTENSION_NAME})
	set_property(TARGET extension.${EXTENSION_NAME} PROPERTY FOLDER "extensions")
endfunction()

function(catapult_define_extension_src EXTENSION_NAME)
	catapult_library_target(catapult.${EXTENSION_NAME} ${ARGN})
	target_link_libraries(catapult.${EXTENSION_NAME} catapult.extensions catapult.handlers catapult.net)
	set_property(TARGET catapult.${EXTENSION_NAME} PROPERTY FOLDER "extensions")
endfunction()

function(catapult_define_extension_test EXTENSION_NAME)
	catapult_test_executable_target(tests.catapult.${EXTENSION_NAME} local ${ARGN})
	set_property(TARGET tests.catapult.${EXTENSION_NAME} PROPERTY FOLDER "extensions")

	# need to promote custom test compiler flags to parent scope
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

add_subdirectory(addressextraction)
add_subdirectory(diagnostics)
add_subdirectory(eventsource)
add_subdirectory(fastfinality)
add_subdirectory(filespooling)
add_subdirectory(harvesting)
add_subdirectory(hashcache)
add_subdirectory(mongo)
add_subdirectory(networkheight)
add_subdirectory(nodediscovery)
add_subdirectory(packetserver)
add_subdirectory(partialtransaction)
add_subdirectory(pluginhandlers)
add_subdirectory(sync)
add_subdirectory(syncsource)
add_subdirectory(storage)
add_subdirectory(timesync)
add_subdirectory(transactionsink)
add_subdirectory(unbondedpruning)
add_subdirectory(zeromq)
