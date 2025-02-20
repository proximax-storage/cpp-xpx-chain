cmake_minimum_required(VERSION 3.2)

set(TARGET_NAME resources.rc)
file(GLOB ${TARGET_NAME}_FILES "*.properties" "*.json")

# Directory where files will be copied
set(COPY_DESTINATION "${CMAKE_BINARY_DIR}/resources.rc")

# Ensure the destination directory exists
file(MAKE_DIRECTORY ${COPY_DESTINATION})

# List to track all copy commands' outputs
set(COPY_OUTPUTS)

# Add a custom command to copy each file
foreach(_file ${${TARGET_NAME}_FILES})
	# Extract just the file name from the full path
	get_filename_component(_filename ${_file} NAME)

	# Define the destination path for the copied file
	set(_destination "${COPY_DESTINATION}/${_filename}")

	# Add the copy command for this file
	add_custom_command(
			OUTPUT ${_destination}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_file} ${_destination}
			DEPENDS ${_file}
			COMMENT "Copying ${_file} to ${_destination}"
	)

	# Add the copied file to the list of outputs
	list(APPEND COPY_OUTPUTS ${_destination})
endforeach()

# Add a custom target that depends on all copied files
add_custom_target(${TARGET_NAME} ALL
		DEPENDS ${COPY_OUTPUTS}
		COMMENT "Copying resource files to ${COPY_DESTINATION}"
		)

# Optionally, display the files being copied
message(STATUS "Copying the following resource files to ${COPY_DESTINATION}:")
foreach(_file ${${TARGET_NAME}_FILES})
	message(STATUS "  ${_file}")
endforeach()