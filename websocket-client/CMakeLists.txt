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

#------------------------------------------------------------------------
# OpenSSL
#
find_package(OpenSSL 1.1 REQUIRED)
if(NOT ${OPENSSL_FOUND})
	message(FATAL_ERROR "OpenSSL not found")
endif()
PRINT(OPENSSL)

add_library(${MyTarget} SHARED
	${websocketclient_HEADERS}
	${websocketclient_SOURCES}
)

#add_dependencies(${MyTarget} MFCLibPlugins MFClibfcs)

set_target_properties(${MyTarget} PROPERTIES PREFIX "")
target_compile_definitions(${MyTarget} PUBLIC WEBSOCKETCLIENT_EXPORTS)

target_include_directories(${MyTarget} PRIVATE
	${OPENSSL_INCLUDE_DIR}
	# ${asio_SOURCE_DIR}/asio/include
	# ${websocketpp_SOURCE_DIR}
)

target_link_libraries(${MyTarget} PRIVATE
	libobs
	MFClibfcs
	MFCLibPlugins
	${OPENSSL_LIBRARIES}
	CURL::libcurl
	nlohmann::json
	chriskohlhoff::asio
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
