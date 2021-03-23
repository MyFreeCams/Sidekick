#######################################
#  websocket-client                   #
#  -(description)                     #
#######################################
#  Target: websocketclient            #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: .              #
#######################################

project(websocketclient)

set(MyTarget "websocketclient")

if(WIN32)
	set(CMAKE_SHARED_LINKER_FLAGS "/DEBUG:FULL")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
	STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#
# Source files.
#
set(websocketclient_HEADERS
	FcsWebsocket.h
	FcsWebsocketImpl.h
	WebsocketClient.h
	WowzaWebsocketClientImpl.h
)
set(websocketclient_SOURCES
	FcsWebsocket.cpp
	FcsWebsocketImpl.cpp
	WebsocketClient.cpp
	WowzaWebsocketClientImpl.cpp
)
if(WIN32)
	set(websocketclient_SOURCES
		${websocketclient_SOURCES}
		dllmain.cpp
	)
endif()

#
# Dependencies
#

#------------------------------------------------------------------------
# Asio C++ Library
#
message(STATUS "Fetching ASIO")
FetchContent_Declare(asio
	GIT_REPOSITORY	https://github.com/chriskohlhoff/asio
	GIT_TAG			asio-1-18-1
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
	FetchContent_Populate(asio)
endif()
if(asio_POPULATED)
	message(STATUS "Fetching ASIO - success")
	add_library(chriskohlhoff::asio INTERFACE IMPORTED)
	target_include_directories(chriskohlhoff::asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
else()
	message(FATAL_ERROR "Fetching ASIO - error")
endif()

#------------------------------------------------------------------------
# CURL
#
find_package(CURL REQUIRED)
PRINT(CURL)

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
# OpenSSL
#
find_package(OpenSSL 1.1 REQUIRED)
if(NOT ${OPENSSL_FOUND})
	message(FATAL_ERROR "OpenSSL not found")
endif()
PRINT(OPENSSL)

#------------------------------------------------------------------------
# WebSocket++ (websocketpp)
#
message(STATUS "Fetching websocketpp")
FetchContent_Declare(websocketpp
	GIT_REPOSITORY 	https://github.com/zaphoyd/websocketpp
	GIT_TAG 		0.8.2
)
FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
	FetchContent_Populate(websocketpp)
endif()
if(websocketpp_POPULATED)
	message(STATUS "Fetching websocketpp - success")
	add_library(zaphoyd::websocketpp INTERFACE IMPORTED)
	target_include_directories(zaphoyd::websocketpp INTERFACE ${websocketpp_SOURCE_DIR})
else()
	message(FATAL_ERROR "Fetching websocketpp - error")
endif()

add_library(${MyTarget} SHARED
	${websocketclient_HEADERS}
	${websocketclient_SOURCES}
)

set_target_properties(${MyTarget} PROPERTIES PREFIX "")
MFCDefines(${MyTarget})
target_compile_definitions(${MyTarget} PUBLIC WEBSOCKETCLIENT_EXPORTS)
target_include_directories(${MyTarget} PRIVATE ${OPENSSL_INCLUDE_DIR})

target_link_libraries(${MyTarget}
	libobs
	MFClibfcs
	MFCLibPlugins
	${OPENSSL_LIBRARIES}
	chriskohlhoff::asio
	CURL::libcurl
	nlohmann::json
	zaphoyd::websocketpp
)

if(APPLE)
	install_obs_plugin(${MyTarget})
elseif(WIN32)
	install_obs_plugin(${MyTarget})

	set(OUTFILE $<TARGET_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH ${OUTFILE} NATIVE_TARGET_FILE)

	set(PDBFILE $<TARGET_PDB_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH "${PDBFILE}" NATIVE_TARGET_PDB)

	# ${CMAKE_BINARY_DIR}/Sidekick/bin
	file(TO_NATIVE_PATH ${MFC_OBS_PLUGIN_BIN_PATH_BUILD} NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD)
	# C:\Users\Public\Sidekick\bin
	# file(TO_NATIVE_PATH ${MFC_OBS_PLUGIN_BIN_PATH} NATIVE_MFC_OBS_PLUGIN_BIN_PATH)

	# create target build & install directories
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

	file(GLOB OPENSSL_BIN_FILES
		"${OPENSSL_INCLUDE_DIR}/../bin${_bin_suffix}/libcrypto*.dll"
		"${OPENSSL_INCLUDE_DIR}/../bin${_bin_suffix}/libssl*.dll"
		"${OPENSSL_INCLUDE_DIR}/../bin/libcrypto*.dll"
		"${OPENSSL_INCLUDE_DIR}/../bin/libssl*.dll"
		"${OPENSSL_INCLUDE_DIR}/bin${_bin_suffix}/libcrypto*.dll"
		"${OPENSSL_INCLUDE_DIR}/bin${_bin_suffix}/libssl*.dll"
		"${OPENSSL_INCLUDE_DIR}/bin/libcrypto*.dll"
		"${OPENSSL_INCLUDE_DIR}/bin/libssl*.dll"
	)

	# copy openssl libs to build root
	foreach(BinFile ${OPENSSL_BIN_FILES})
		file(TO_NATIVE_PATH ${BinFile} NATIVE_BinFile)
		message(STATUS "Copying ${NATIVE_BinFile} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD}")
		file(COPY ${NATIVE_BinFile} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD})
	endforeach()

	# copy openssl libs to install root
	# foreach(BinFile ${OPENSSL_BIN_FILES})
	# 	file(TO_NATIVE_PATH ${BinFile} NATIVE_BinFile)
	# 	message(STATUS "Copying ${NATIVE_BinFile} => ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH}")
	# 	file(COPY ${NATIVE_BinFile} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH})
	# endforeach()

	#file(COPY ${LIBRESTCLIENT_CPP_LIBRARY} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH_BUILD})
	#file(COPY ${LIBRESTCLIENT_CPP_LIBRARY} DESTINATION ${NATIVE_MFC_OBS_PLUGIN_BIN_PATH})
endif()
