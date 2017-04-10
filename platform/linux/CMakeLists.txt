include_directories(../../platform/linux/)

qi_create_lib(platform_linux SHARED
              gestures/LinuxSpeechGesture.cpp
	          sensors/LinuxMicrophone.cpp)

qi_use_lib(platform_linux OPENCV2_CORE OPENCV2_HIGHGUI self)

qi_stage_lib(platform_linux)
