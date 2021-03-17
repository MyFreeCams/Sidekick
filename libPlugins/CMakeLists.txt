#######################################
#  libPlugins                         #
#  -shared lib for files in solution  #
#######################################
#  Target: MFCLibPlugins              #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(MyTarget "MFCLibPlugins")

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#
# Source files.
#
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/build_version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/build_version.h
)

find_package(Qt5 COMPONENTS Core Widgets Gui Svg REQUIRED)
PRINT(Qt5)

set(SRC_OBS_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/build_version.h
	CollectSystemInfo.h
	CollectSystemInfo.cpp
	EdgeChatSock.h
	EdgeChatSock.cpp
	HttpRequest.h
	HttpRequest.cpp
	IPCShared.h
	IPCShared.cpp
	MFCConfigConstants.h
	MFCEdgeIngest.h
	MFCEdgeIngest.cpp
	MFCPluginAPI.h
	MFCPluginAPI.cpp
	ObsServicesJson.h
	ObsServicesJson.cpp
	ObsUtil.h
	ObsUtil.cpp
	Portable.h
	SidekickModelConfig.h
	SidekickModelConfig.cpp
)
set(SRC_OBS_Win
	SysParam.h
	SysParam.cpp
	targetver.h
)
set(SRC_OBS_Mac
	SysParam_Mac.h
	SysParam_Mac.cpp
)

if(APPLE)
	set_source_files_properties(${SRC_OBS_COMMON} PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
endif()

#
# include directories for target_include_directories
#
set(MyTarget_Include
)
set(MyTarget_Include_Win
)
set(MyTarget_Include_Mac
)

#
# defines for target_compile_definitions
#
set(MyTarget_DEFINES
)
set(MyTarget_DEFINES_Win
)
set(MyTarget_DEFINES_Mac
)

#
# compile options.
#
set(MyTarget_OPTIONS
)
set(MyTarget_OPTIONS_Win
	-D_CRT_SECURE_NO_WARNINGS
	-D_UNICODE
	-DUNICODE
)
set(MyTarget_OPTIONS_Mac
	-Wunused-value
)

if(APPLE)
	add_library(${MyTarget} STATIC
		${SRC_OBS_COMMON}
		${SRC_OBS_Mac}
	)
	target_include_directories(${MyTarget} PUBLIC
		${MyTarget_Include}
		${MyTarget_Include_Mac}
	)
	target_compile_options(${MyTarget} PUBLIC
		${MyTarget_OPTIONS}
		${MyTarget_OPTIONS_Mac}
	)
elseif(WIN32)
	add_library(${MyTarget} STATIC
		${SRC_OBS_COMMON}
		${SRC_OBS_Win}
	)
	target_include_directories(${MyTarget} PUBLIC
		${MyTarget_Include}
		${MyTarget_Include_Win}
	)
	target_compile_options(${MyTarget} PUBLIC
		${MyTarget_OPTIONS}
		${MyTarget_OPTIONS_Win}
	)
endif()

target_compile_definitions(${MyTarget} PUBLIC
	BOOST_ALL_NO_LIB
)

#add_dependencies(${MyTarget} obs)

target_link_libraries(${MyTarget} PRIVATE
	CURL::libcurl
	obs-frontend-api
	Qt5::Widgets
	Qt5::Gui
	Qt5::Svg
	nlohmannjson
)

MFCDefines(${MyTarget})
