
set(MyTarget libipc)

project(${MyTarget})

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
		IPCUtil.h
		IPCUtil.cpp
		mfc_ipc.h
		ProcessRecord.h
		Router.h
		Router.cpp
		IPCSemaphore.h
		IPCSemaphore.cpp
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
set(SRC_LIB_MSG_QUE_Lnx
		ShmemVector.h ShmemDefines.h ShmemBase.h ShmemSimpleType.h ShmemString.h MFC_IPC.h)

if(APPLE)
	add_library(${MyTarget} SHARED
		${SRC_LIB_MSG_QUE}
		${SRC_LIB_MSG_QUE_Mac}
	)
	target_link_libraries(${MyTarget}
			${Boost_LIBRARIES}
			MFClibfcs
			)
elseif(WIN32)
	add_library(${MyTarget} STATIC
		${SRC_LIB_MSG_QUE}
		${SRC_LIB_MSG_QUE_Win}
	)
else()
	add_library(${MyTarget} STATIC
		${SRC_LIB_MSG_QUE}
		${SRC_LIB_MSG_QUE_Lnx}
	)
endif()

SET_TARGET_PROPERTIES(${MyTarget} PROPERTIES PREFIX "")
target_include_directories(${MyTarget} PUBLIC
		#${CMAKE_CURRENT_SOURCE_DIR}
		#${CMAKE_CURRENT_BINARY_DIR}
		#${CMAKE_CURRENT_BINARY_DIR}/..
		#${CMAKE_CURRENT_SOURCE_DIR}/..
		../libfcs
	${Boost_INCLUDE_DIRS}
	${nlohmannjson_SOURCE_DIR}/single_include
	../libipc
)

target_compile_definitions(${MyTarget} PUBLIC
		BOOST_ALL_NO_LIB

		)

target_link_libraries(${MyTarget}

)


if(APPLE)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
#	target_compile_options(${MyTarget} PUBLIC -fexceptions -Wno-error -Weverything -Wno-unused-parameter -Wno-unused-variable -fvisibility=hidden -fvisibility-inlines-hidden)
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
