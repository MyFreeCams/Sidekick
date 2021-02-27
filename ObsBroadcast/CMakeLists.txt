#######################################
#  ObsBroadcast                       #
#  -(description)                     #
#######################################
#  Target: MFCBroadCast               #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(MyTarget "MFCBroadcast")

if(WIN32)
	set(CMAKE_SHARED_LINKER_FLAGS "/DEBUG:FULL")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
	STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

# Needed for Q_OBJECT macro to trigger generation of moc headers
set(CMAKE_AUTOMOC ON)
# Needed for dllmain.qrc file to be compiled to rcc data and then turned into
# generated cpp source to be included in the binary output as a Qt resource
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#
# Source files.
#

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/BroadcastPlugin.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/BroadcastPlugin.rc
)

# core files (non-webrtc).
set(MyTarget_CORE_FILES
	BroadcastPluginStart.cpp
	HttpThread.h
	HttpThread.cpp
	ObsBroadcast.h
	ObsBroadcast.cpp
	ObsCallbackEvent.h
	ObsCallbackEvent.cpp
	SanitizeInputs.h
	SDPUtil.h
	SidekickProperties.h
	SidekickProperties.cpp
	SidekickTypes.h
	webrtc_version.h
	wowza-stream.h
	wowza-stream.cpp
)

# webrtc files.
set(MyTarget_WEBRTC_FILES
	ADMWrapper.h
	ADMWrapper.cpp
	EncoderFactory.h
	EncoderFactory.cpp
	NV12Buf.h
	NV12Buf.cpp
	VideoTrackSource.h
	VideoTrackSource.cpp
	WebRTCStream.h
	WebRTCStream.cpp
	X264Encoder.h
	X264Encoder.cpp
)
source_group(WebRTC FILES ${MyTarget_WEBRTC_FILES})
if(APPLE)
	set_source_files_properties(${MyTarget_WEBRTC_FILES} PROPERTIES
		COMPILE_FLAGS " -fvisibility=hidden -fvisibility-inlines-hidden"
	)
endif()

# add platform sources.
if(APPLE)
	set(MyTarget_DLLMAIN_FILES
		dllmain.qrc
	)
elseif(WIN32)
	set(MyTarget_DLLMAIN_FILES
		dllmain.cpp
		dllmain.qrc
	)
	set(MyTarget_RESOURCE_FILES
		${CMAKE_CURRENT_BINARY_DIR}/BroadcastPlugin.rc
		resource.h
		targetver.h
	)
endif()

#
# Dependencies
#

#------------------------------------------------------------------------
# WebRTC
#
find_package(WebRTC REQUIRED)
PRINT(WEBRTC)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/webrtc_version.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/webrtc_version.h"
)

#------------------------------------------------------------------------
# x264
#
find_package(Libx264 REQUIRED)
PRINT(LIBX264)

# add platform link dependancies.
if(APPLE)
	set(MyTarget_PLATFORM_LIBRARIES
		objc
	)
elseif(WIN32)
	set(MyTarget_PLATFORM_LIBRARIES
		Iphlpapi
		version
		Wbemuuid
		Winhttp
		Wininet
		winmm
		ws2_32
	)
	set(MyTarget_PLATFORM_LINK_DIRS
		${CMAKE_BINARY_DIR}/UI/obs-frontend-api/$<CONFIG>
		${CMAKE_BINARY_DIR}/libobs/$<CONFIG>
	)
	if(MSVC)
		set(MyTarget_PLATFORM_LIBRARIES
			${MyTarget_PLATFORM_LIBRARIES}
			w32-pthreads
		)
	endif()
endif()

# create library target
add_library(${MyTarget} MODULE
	${MyTarget_CORE_FILES}
	${MyTarget_WEBRTC_FILES}
	${MyTarget_DLLMAIN_FILES}
	${MyTarget_RESOURCE_FILES}
)

#add_dependencies(${MyTarget} obs obs-frontend-api MFCLibPlugins)

target_include_directories(${MyTarget} PRIVATE
	${WEBRTC_INCLUDE_DIRS}
	${LIBX264_INCLUDE_DIRS}
)

target_compile_definitions(${MyTarget} PUBLIC
	# BOOST_ALL_NO_LIB
	#BOOST_LIB_DIAGNOSTIC
)
if (APPLE)
	#less visibiilty issues with this
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
	#target_compile_options(${MyTarget} PUBLIC -fexceptions -Wno-error -Wno-unused-parameter -Wno-unused-variable -fvisibility=hidden -fvisibility-inlines-hidden)
	# removed visibility 2/15 9:47am
	target_compile_options(${MyTarget} PUBLIC -fexceptions -Wno-error -Wno-unused-parameter -Wno-unused-variable )

endif ()
if(WIN32)
	target_compile_options(${MyTarget} PUBLIC "/GS-")
endif()

MFCDefines(${MyTarget})

#
# linking
#
target_link_libraries(${MyTarget} PRIVATE
	libobs
	obs-frontend-api
	MFClibfcs
	MFCLibPlugins
	websocketclient
	CURL::libcurl
	Qt5::Widgets
	Qt5::Gui
	Qt5::Svg
	WebRTC::WebRTC
	${LIBX264_LIBRARIES}
	${MyTarget_PLATFORM_LIBRARIES}
	# ${Boost_LIBRARIES}
	Boost::boost
	libipc
	#/Users/toddanderson/mfc-dev/obs-studio/xAuto/plugins/myfreecams/sidekick/libipc/libipc.a
)

target_link_directories(${MyTarget} PRIVATE
	${MyTarget_PLATFORM_LINK_DIRS}
)

#
# install
#
if(APPLE)
	install_obs_plugin(${MyTarget})
elseif(WIN32)
	# 1) turn "MFCBroadcast" into "ObsBroadcast", same with "MFCUpdater"/"ObsUpdater",
	# 2) convert cmake path to native filesystem path for windows,
	# 3) copy output file and pdb if it exists to the sidekick bindir

	install_obs_plugin(${MyTarget})

	STRING(REGEX REPLACE "MFC" "Obs" PROJECT_DIRNAME ${MyTarget})

	# set(OUTFILE "${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/${PROJECT_DIRNAME}/$<CONFIG>/${MyTarget}.dll")
	set(OUTFILE $<TARGET_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH "${OUTFILE}" NATIVE_TARGET_FILE)

	set(PDBFILE $<TARGET_PDB_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH "${PDBFILE}" NATIVE_TARGET_PDB)

	# ${CMAKE_BINARY_DIR}/Sidekick/bin
	file(TO_NATIVE_PATH ${MFC_OBS_PLUGIN_BIN_PATH_BUILD} NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD)
	# C:\Users\Public\Sidekick\bin
	# file(TO_NATIVE_PATH ${MFC_OBS_PLUGIN_BIN_PATH} NATIVE_MFC_OBS_PLUGIN_BIN_PATH)

	# create build/install directories
	add_custom_command(TARGET ${MyTarget}
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}
		# COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}
	)

	# copy target to build directory
	message(STATUS "OUTPUT COPY: ${NATIVE_TARGET_FILE} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}")
	message(STATUS "PDB    COPY: ${NATIVE_TARGET_PDB} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}")
	add_custom_command(TARGET ${MyTarget}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_FILE} ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}
		COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_PDB} ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}
	)

	# copy target to install directory
	# message(STATUS "OUTPUT COPY: ${NATIVE_TARGET_FILE} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}")
	# message(STATUS "PDB    COPY: ${NATIVE_TARGET_PDB} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}")
	# add_custom_command(TARGET ${MyTarget}
	# 	POST_BUILD
	# 	COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_FILE} ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}
	# 	COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_PDB} ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}
	# )

	# set(C_RUNTIME_FILES
	# 	"${CMAKE_CURRENT_SOURCE_DIR}/../Resources/msvcp140.dll"
	# 	"${CMAKE_CURRENT_SOURCE_DIR}/../Resources/msvcr120.dll"
	# 	"${CMAKE_CURRENT_SOURCE_DIR}/../Resources/vcruntime140.dll"
	# 	"${CMAKE_CURRENT_SOURCE_DIR}/../Resources/vcruntime140_1.dll"
	# )

	# copy c runtime dlls to build & install directories
	# foreach(BinFile ${C_RUNTIME_FILES})
	# 	file(TO_NATIVE_PATH ${BinFile} NATIVE_BinFile)
	# 	message(STATUS "Copying ${NATIVE_BinFile} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}")
	# 	file(COPY ${NATIVE_BinFile} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD})
	# 	message(STATUS "Copying ${NATIVE_BinFile} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}")
	# 	file(COPY ${NATIVE_BinFile} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH})
	# endforeach()

	# file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../Resources/obs_broadcast.rcc NATIVE_RCC_FILE)

	# message(STATUS "Copying ${NATIVE_RCC_FILE} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}")
	# file(COPY ${NATIVE_RCC_FILE} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD})
	# message(STATUS "Copying ${NATIVE_RCC_FILE} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}")
	# file(COPY ${NATIVE_RCC_FILE} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH})
endif()



