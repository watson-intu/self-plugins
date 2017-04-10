if("$ENV{TARGET}" STREQUAL "")
	message(warning " ---- No Target defined, defaulting to Nao.")
	set(ENV{TARGET} "nao")
endif()

if("$ENV{TARGET}" STREQUAL "nao")
	add_subdirectory(nao)
endif()

if("$ENV{TARGET}" STREQUAL "linux")
	add_subdirectory(linux)
endif()

if("$ENV{TARGET}" STREQUAL "raspi")
	add_subdirectory(raspi)
endif()

if("$ENV{TARGET}" STREQUAL "sota")
	add_subdirectory(sota)
endif()

if("$ENV{TARGET}" STREQUAL "mac")
	add_subdirectory(mac)
endif()

