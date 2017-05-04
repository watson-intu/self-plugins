set(CMAKE_MACOSX_RPATH 1)
include_directories(".")

qi_create_lib(platform_mac SHARED
        MacPlatform.cpp
        gestures/AQSpeechGesture.cpp
        gestures/MacSoundGesture.cpp
        sensors/AQMicrophone.cpp
        services/MacBrowser.cpp
        tests/TestMacSoundGesture.cpp)

qi_use_lib(platform_mac self utils portaudio)

qi_stage_lib(platform_mac)
