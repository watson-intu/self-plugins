add_definitions(" -DAUDIOIMPL_IS_REMOTE -DNAO_ENABLED -DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")
include_directories(. ../../lib)

file(GLOB_RECURSE SERVICES_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "services/*.cpp")
file(GLOB_RECURSE FLANN_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "lib/flann/src/*.cpp")
list(REMOVE_ITEM FLANN_CPP "lib/flann/src/mpi/*.cpp" )
file(GLOB_RECURSE FLANN_C RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "lib/flann/src/*.c")
file(GLOB_RECURSE PCL_COMMON_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "lib/plc/common/src/*.cpp")
file(GLOB_RECURSE PCL_COMMON_C RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "lib/plc/common/src/*.c")

qi_create_lib(pcl_plugin SHARED 
	${SERVICES_CPP} 
	${FLANN_CPP} 
	${FLANN_C} 
	${PCL_COMMON_CPP} 
	${PCL_COMMON_C}
	)
qi_use_lib(pcl_plugin self utils tinythread++)
qi_stage_lib(pcl_plugin)

