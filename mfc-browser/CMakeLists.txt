cmake_minimum_required(VERSION 2.8.12)

# find_package now searches prefixes specified by the
#   <PackageName>_ROOT CMake variable and the
#   <PackageName>_ROOT environment variable.
cmake_policy(SET CMP0074 NEW)

set(MyTarget mfc-browser)
set(MyTargetPage mfc-browser-page)
project(${MyTarget})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/panel")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")
include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_BINARY_DIR}/UI")
include_directories("${CMAKE_SOURCE_DIR}/UI")

find_package(CEF QUIET)

if(NOT CEF_FOUND)
	message(STATUS "CEF Not found -- mfc-browser plugin disabled.")
	return()
endif()

option(EXPERIMENTAL_SHARED_TEXTURE_SUPPORT_ENABLED "Enable shared texture support for the browser plugin (Win32)" OFF)
option(BROWSER_PANEL_SUPPORT_ENABLED "Enables Qt web browser panel support" ON)

if(NOT APPLE)
	option(USE_QT_LOOP "Runs CEF on the main UI thread alongside Qt instead of in its own thread" OFF)
else()
	set(USE_QT_LOOP TRUE)
endif()

if(APPLE)
	option(BROWSER_DEPLOY "Marks full deployment of browser plugin" OFF)
	if(BROWSER_DEPLOY)
		add_definitions(-DBROWSER_DEPLOY)
	endif()
endif()

# add boost
#include_directories(${Boost_INCLUDE_DIR})
#link_directories(${Boost_LIBRARY_DIRS})

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/browser-config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/browser-config.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CEF_ROOT_DIR}")

# ----------------------------------------------------------------------------

set(obs-browser_LIBRARIES
	libobs
	obs-frontend-api
	)

list(APPEND obs-browser_LIBRARIES
	${CEF_LIBRARIES})

if(BROWSER_PANEL_SUPPORT_ENABLED OR USE_QT_LOOP)
	if(DEFINED QTDIR${_lib_suffix})
		list(APPEND CMAKE_PREFIX_PATH "${QTDIR${_lib_suffix}}")
	elseif(DEFINED QTDIR)
		list(APPEND CMAKE_PREFIX_PATH "${QTDIR}")
	elseif(DEFINED ENV{QTDIR${_lib_suffix}})
		list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR${_lib_suffix}}")
	elseif(DEFINED ENV{QTDIR})
		list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
	endif()

	set(CMAKE_AUTOMOC TRUE)
	find_package(Qt5Widgets REQUIRED)
	list(APPEND obs-browser_LIBRARIES
		Qt5::Widgets
		Qt5::Svg
		)
endif()

if(MSVC)
	string(REPLACE "/MD" "/MT"
		"CMAKE_C_FLAGS"
		"${CMAKE_C_FLAGS}")

	string(REPLACE "/MD" "/MT"
		"CMAKE_CXX_FLAGS"
		"${CMAKE_CXX_FLAGS}")

	string(TOUPPER "${CMAKE_CONFIGURATION_TYPES}" UPPER_CONFIG_TYPES)
	foreach(CONFIG_TYPE ${UPPER_CONFIG_TYPES})
		string(REPLACE "/MD" "/MT"
			"CMAKE_C_FLAGS_${CONFIG_TYPE}"
			"${CMAKE_C_FLAGS_${CONFIG_TYPE}}")

		string(REPLACE "/MD" "/MT"
			"CMAKE_CXX_FLAGS_${CONFIG_TYPE}"
			"${CMAKE_CXX_FLAGS_${CONFIG_TYPE}}")
	endforeach()
	list(APPEND obs-browser_LIBRARIES
		d3d11
		dxgi
		)
endif()

set(obs-browser_SOURCES
	obs-browser-source.cpp
	obs-browser-source-audio.cpp
	obs-browser-plugin.cpp
	browser-scheme.cpp
	browser-client.cpp
	browser-app.cpp
	deps/json11/json11.cpp
	deps/base64/base64.cpp
	deps/wide-string.cpp
	MfcJsExtensions.cpp
	MFCBrowserPanel.cpp
	${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp
	)
set(obs-browser_HEADERS
	obs-browser-source.hpp
	browser-scheme.hpp
	browser-client.hpp
	browser-app.hpp
	browser-version.h
	deps/json11/json11.hpp
	deps/base64/base64.hpp
	deps/wide-string.hpp
	cef-headers.hpp
	MfcJsExtensions.h
	${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp
	)

# MFC browser add ons to stop obs-browser
set(MFC_BROWSER_ADDONS
	mfc-browser.rc
	)
set(MFC_BROWSER_PAGE_ADDONS
	mfc-browser-page.rc
	)

# only allow browser panels on win32 for now -- other operating systems
# need more testing
if(WIN32 AND BROWSER_PANEL_SUPPORT_ENABLED)
	list(APPEND obs-browser_SOURCES
		panel/browser-panel.cpp
		panel/browser-panel-client.cpp
		)
	list(APPEND obs-browser_HEADERS
		panel/browser-panel.hpp
		panel/browser-panel-client.hpp
		panel/browser-panel-internal.hpp
		)
endif()

add_library(${MyTarget} MODULE
	${obs-browser_SOURCES}
	${obs-browser_HEADERS}
	${MFC_BROWSER_ADDONS}
	)

#add_dependencies(${MyTarget} obs)

MFCDefines(${MyTarget})

target_link_libraries(${MyTarget}
	${obs-browser_LIBRARIES}
	Boost::Boost
	)
if(USE_QT_LOOP)
	target_compile_definitions(${MyTarget} PRIVATE USE_QT_LOOP)
endif()

# ----------------------------------------------------------------------------

set(obs-browser-page_SOURCES
	obs-browser-page/obs-browser-page-main.cpp
	browser-app.cpp
	deps/json11/json11.cpp
	MfcJsExtensions.cpp
	)
set(obs-browser-page_HEADERS
	obs-browser-page/obs-browser-page-main.cpp
	browser-app.hpp
	deps/json11/json11.hpp
	cef-headers.hpp
	MfcJsExtensions.h
	)

add_executable(${MyTargetPage}
	${obs-browser-page_SOURCES}
	${obs-browser-page_HEADERS}
	${MFC_BROWSER_PAGE_ADDONS}
	)

MFCDefines(${MyTargetPage})

target_link_libraries(${MyTargetPage}
	${CEF_LIBRARIES}
	)

if (APPLE)
	set_target_properties(${MyTargetPage} PROPERTIES
		COMPILE_FLAGS "-mmacosx-version-min=10.8")
endif(APPLE)

if (WIN32)
	set_target_properties(${MyTargetPage} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif(WIN32)

set(OUTPUT_FILENAME "obs-browser")
set_target_properties(${MyTarget} PROPERTIES OUTPUT_NAME "${OUTPUT_FILENAME}")
set_target_properties(${MyTargetPage} PROPERTIES OUTPUT_NAME "${OUTPUT_FILENAME}-page")
# ----------------------------------------------------------------------------
set(SIDEKICK_INSTALL_PATH "${MFC_OBS_PLUGIN_BIN_PATH}/")
option(SIDEKICK_INSTALL "Install ${MyTarget} in Sidekick deployment path: ${SIDEKICK_INSTALL_PATH}" OFF)

if (WIN32)
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CEF_ROOT_DIR}/Resources"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
	)

	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/chrome_elf.dll"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libcef.dll"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libEGL.dll"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libGLESv2.dll"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/natives_blob.bin"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/snapshot_blob.bin"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/v8_context_snapshot.bin"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
	)

	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}.pdb"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/${OUTPUT_FILENAME}.pdb"
	)
	add_custom_command(TARGET ${MyTargetPage} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}-page.pdb"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/${OUTPUT_FILENAME}-page.pdb"
	)

	if (SIDEKICK_INSTALL)
		message(STATUS "Installing ${MyTarget} in Sidekick deployment path: ${SIDEKICK_INSTALL_PATH}")
		add_custom_command(TARGET ${MyTarget} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CEF_ROOT_DIR}/Resources"
			"${SIDEKICK_INSTALL_PATH}"
		)

		add_custom_command(TARGET ${MyTarget} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/chrome_elf.dll"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/libcef.dll"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/libEGL.dll"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/libGLESv2.dll"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/natives_blob.bin"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/snapshot_blob.bin"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CEF_ROOT_DIR}/Release/v8_context_snapshot.bin"
			"${SIDEKICK_INSTALL_PATH}"
		)

		# mfc-browser needs to load AFTER obs-browser
		add_custom_command(TARGET ${MyTarget} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}.dll"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}.pdb"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/mfc-browser.rc"
			"${SIDEKICK_INSTALL_PATH}/${OUTPUT_FILENAME}.rc"
		)
		add_custom_command(TARGET ${MyTargetPage} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}-page.exe"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}-page.pdb"
			"${SIDEKICK_INSTALL_PATH}"
			COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/mfc-browser-page.rc"
			"${SIDEKICK_INSTALL_PATH}/${OUTPUT_FILENAME}-page.rc"
		)
	else()
		message(STATUS "Not installing ${MyTarget} in Sidekick deployment path: ${SIDEKICK_INSTALL_PATH}")
	endif()

	# Create Sidekick build directory (used when creating installer)
	add_custom_command(TARGET ${MyTarget} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${MFC_OBS_PLUGIN_BIN_PATH_BUILD}
	)
	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CEF_ROOT_DIR}/Resources"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
	)
	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/chrome_elf.dll"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libcef.dll"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libEGL.dll"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/libGLESv2.dll"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/natives_blob.bin"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/snapshot_blob.bin"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CEF_ROOT_DIR}/Release/v8_context_snapshot.bin"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
	)

	# Copy targets to Sidekick build directory (for installer)
	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}.dll"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}.pdb"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/mfc-browser.rc"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/${OUTPUT_FILENAME}.rc"
	)
	add_custom_command(TARGET ${MyTargetPage} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}-page.exe"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/mfc-browser/$(Configuration)/${OUTPUT_FILENAME}-page.pdb"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/mfc-browser-page.rc"
		"${MFC_OBS_PLUGIN_BIN_PATH_BUILD}/${OUTPUT_FILENAME}-page.rc"
	)
endif()

if(APPLE AND NOT BROWSER_DEPLOY)
	add_custom_command(TARGET ${MyTarget} POST_BUILD
		COMMAND "install_name_tool" -change
		"@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework"
		"${CEF_LIBRARY}/Chromium Embedded Framework"
		"mfc-browser.so")
	add_custom_command(TARGET ${MyTargetPage} POST_BUILD
		COMMAND "install_name_tool" -change
		"@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework"
		"${CEF_LIBRARY}/Chromium Embedded Framework"
		"mfc-browser-page")
endif()

if (APPLE OR NOT SIDEKICK_INSTALL)
	install_obs_plugin_with_data(${MyTarget} data)
	install_obs_plugin(${MyTargetPage})
endif()
