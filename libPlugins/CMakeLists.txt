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
	set_source_files_properties(${SRC_OBS_COMMON} ${SRC_OBS_Mac} PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
endif()

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
	set(SRC_OBS
		${SRC_OBS_COMMON}
		${SRC_OBS_Mac}
	)
	set(MyTarget_OPTIONS
		${MyTarget_OPTIONS}
		${MyTarget_OPTIONS_Mac}
	)
elseif(WIN32)
	set(SRC_OBS
		${SRC_OBS_COMMON}
		${SRC_OBS_Win}
	)
	set(MyTarget_OPTIONS
		${MyTarget_OPTIONS}
		${MyTarget_OPTIONS_Win}
	)
endif()

#
# Dependencies
#

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

add_library(${MyTarget} STATIC ${SRC_OBS})
MFCDefines(${MyTarget})
target_compile_options(${MyTarget} PUBLIC ${MyTarget_OPTIONS})
# target_compile_definitions(${MyTarget} PUBLIC BOOST_ALL_NO_LIB)

target_link_libraries(${MyTarget} PUBLIC
	obs-frontend-api
	Boost::headers
	Boost::date_time
	Boost::system
	# Boost::thread
	# Boost::disable_autolinking
	CURL::libcurl
	Qt5::Widgets
	Qt5::Gui
	Qt5::Svg
	nlohmann::json
)
