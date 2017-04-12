include_directories(".")

add_subdirectory(smartthings)
add_subdirectory(remote)
add_subdirectory(opencv)
add_subdirectory(lib)

add_subdirectory(platform)
if(EXISTS lab)
	add_subdirectory(lab)
endif()

