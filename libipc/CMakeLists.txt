#######################################
#  libipc                             #
#  -(description)                     #
#######################################
#  Target: libipc                     #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ./             #
#######################################

set(MyTarget libipc)

# project(${MyTarget})

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(SRC_LIB_MSG_QUE
	EventList.h
	EventList.cpp
	EvtDefines.h
	EvtDefines.cpp
	IPCEvent.h
	IPCEvent.cpp
	IPCEventHandler.h
	IPCEventHandler.cpp
	IPCSemaphore.h
	IPCSemaphore.cpp
	IPCUtil.h
	IPCUtil.cpp
	mfc_ipc.h
	ProcessRecord.h
	Router.h
	Router.cpp
	ShmemBase.h
	ShmemContainer.h
	ShmemDefines.h
	ShmemManager.h
	ShmemManager.cpp
	ShmemSignal.h
	ShmemSimpleType.h
	ShmemVector.h
)

set(SRC_LIB_MSG_QUE_Win
)
set(SRC_LIB_MSG_QUE_Mac
)

if(APPLE)
	find_package(Threads REQUIRED)
	add_library(${MyTarget} SHARED
		${SRC_LIB_MSG_QUE}
		${SRC_LIB_MSG_QUE_Mac}
	)
	target_link_libraries(${MyTarget} PRIVATE
		# ${Boost_LIBRARIES}
		Boost::boost
		Boost::thread
		Boost::program_options
		Boost::date_time
		Boost::system
		Boost::filesystem
		Boost::chrono
		MFClibfcs
	)
elseif(WIN32)
	add_library(${MyTarget} STATIC
		${SRC_LIB_MSG_QUE}
		${SRC_LIB_MSG_QUE_Win}
	)
endif()

SET_TARGET_PROPERTIES(${MyTarget} PROPERTIES PREFIX "")
target_include_directories(${MyTarget} PUBLIC
	#${CMAKE_CURRENT_SOURCE_DIR}
	#${CMAKE_CURRENT_BINARY_DIR}
	#${CMAKE_CURRENT_BINARY_DIR}/..
	#${CMAKE_CURRENT_SOURCE_DIR}/..
	${Boost_INCLUDE_DIRS}
	${nlohmannjson_SOURCE_DIR}/single_include
)

target_compile_definitions(${MyTarget} PUBLIC
	# BOOST_ALL_NO_LIB
)

if(APPLE)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
	# everything in libipc is in the namespace MFCIPC so no need to hide functions. Leave visibility as is.
	target_compile_options(${MyTarget} PUBLIC -fexceptions -Wno-error -Wno-unused-parameter -Wno-unused-variable)
elseif(WIN32)
	# Turn on RTTI for all files in this directory.
	add_definitions("/GR")
	# Disable warnings as errors.
	target_compile_options(${MyTarget} PUBLIC /WX-)
else()
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
	add_compile_options(-Wno-error -Wno-unused-parameter -frtti -fexceptions)
	target_compile_options(${MyTarget} PUBLIC -fexceptions -frtti -Wno-unused-variable)
endif()

if(APPLE)
	install_obs_plugin(${MyTarget})
endif()
