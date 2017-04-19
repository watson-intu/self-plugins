include_directories(".")

add_subdirectory(lib)

add_subdirectory(iva)
if(EXISTS lab)
	add_subdirectory(lab)
endif()
add_subdirectory(nexmo)
add_subdirectory(opencv)
add_subdirectory(platform)
add_subdirectory(remote)
add_subdirectory(rg)
add_subdirectory(smartthings)
add_subdirectory(watson)
add_subdirectory(wayblazer)

