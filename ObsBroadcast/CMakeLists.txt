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
	set_source_files_properties(${MyTarget_WEBRTC_FILES} PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
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
# Boost
#
set(Boost_USE_DEBUG_LIBS OFF)
set(Boost_USE_RELEASE_LIBS ON)
#set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_DEBUG_RUNTIME OFF)
#set(BOOST_NO_EXCEPTIONS ON)
find_package(Boost 1.75 COMPONENTS date_time thread system REQUIRED)
PRINT(Boost)

#------------------------------------------------------------------------
# Nlohmann JSON (Modern JSON for C++)
#
FetchContent_Declare(nlohmannjson
	URL			https://github.com/nlohmann/json/releases/download/v3.9.1/include.zip
	URL_HASH	SHA256=6bea5877b1541d353bd77bdfbdb2696333ae5ed8f9e8cc22df657192218cad91
)
FetchContent_GetProperties(nlohmannjson)
if(NOT nlohmannjson_POPULATED)
	message(STATUS "Fetching nlohmann json")
	FetchContent_Populate(nlohmannjson)
	if(nlohmannjson_POPULATED)
		message(STATUS "Fetching nlohmann json - success")
	else()
		message(FATAL_ERROR "Fetching nlohmann json - error")
	endif()
endif()
if(nlohmannjson_POPULATED)
	if(NOT DEFINED nlohman::json)
		add_library(nlohmann::json INTERFACE IMPORTED)
		target_include_directories(nlohmann::json INTERFACE ${nlohmannjson_SOURCE_DIR}/single_include)
	endif()
endif()

#------------------------------------------------------------------------
# Qt
#
find_package(Qt5 COMPONENTS Core Widgets Gui Svg REQUIRED)
PRINT(Qt5)

#------------------------------------------------------------------------
# WebRTC
#
find_package(WebRTC REQUIRED)
PRINT(WebRTC)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/webrtc_version.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/webrtc_version.h"
)

#------------------------------------------------------------------------
# x264
#
find_package(Libx264 REQUIRED)
PRINT(Libx264)

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

target_include_directories(${MyTarget} PRIVATE
	${WEBRTC_INCLUDE_DIRS}
	${LIBX264_INCLUDE_DIRS}
)

MFCDefines(${MyTarget})

if(WIN32)
	target_compile_options(${MyTarget} PUBLIC "/GS-")
endif()

target_link_directories(${MyTarget} PRIVATE ${MyTarget_PLATFORM_LINK_DIRS})

target_link_libraries(${MyTarget}
	libobs
	obs-frontend-api
	MFClibfcs
	MFCLibPlugins
	websocketclient
	Boost::headers
	Boost::date_time
	Boost::system
	# Boost::disable_autolinking
	CURL::libcurl
	nlohmann::json
	Qt5::Widgets
	Qt5::Gui
	Qt5::Svg
	WebRTC::WebRTC
	${LIBX264_LIBRARIES}
	${MyTarget_PLATFORM_LIBRARIES}
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
