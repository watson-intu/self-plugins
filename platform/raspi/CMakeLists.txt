include_directories(. wiringPi)

SET(GCC_COVERAGE_LINK_FLAGS    "-lwiringPi")
add_definitions(${GCC_COVERAGE_LINK_FLAGS})

qi_create_lib(platform_raspi SHARED
        gestures/RaspiAnimateGesture.cpp
        gestures/RaspiSpeechGesture.cpp
        sensors/RaspiMicrophone.cpp)

target_link_libraries(platform_raspi wiringPi)

qi_use_lib(platform_raspi self)

qi_stage_lib(platform_raspi)