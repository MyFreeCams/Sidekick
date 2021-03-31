function(MFCDefines TheTarget)
	message(STATUS "MFCDefines: ${TheTarget}")
	#
	#defines for target_compile_definitions
	#
	set(MFC_DEFINES
		MFC_AGENT_EDGESOCK=${MFC_AGENT_EDGESOCK}
		MFC_BROWSER_AVAILABLE=${MFC_BROWSER_AVAILABLE}
		MFC_BROWSER_LOGIN=${MFC_BROWSER_LOGIN}
		MFC_LOG_LEVEL=${MFC_LOG_LEVEL}
		MFC_LOG_OUTPUT_MASK=${MFC_LOG_OUTPUT_MASK}
		MFC_NO_UPDATES=${MFC_NO_UPDATES}
		MFC_OBS_INSTALL_PATH="${MFC_OBS_PLUGIN_BIN_PATH}"
		MFC_OBS_PLUGIN_BIN_PATH="${MFC_OBS_PLUGIN_BIN_PATH}"
		MFC_OBS_PLUGIN_BIN_PATH_BUILD="${MFC_OBS_PLUGIN_BIN_PATH_BUILD}"
		MFC_OBS_PLUGIN_LOG_PATH="${MFC_OBS_PLUGIN_LOG_PATH}"
		MFC_PLATFORM="${_lib_suffix}bit"
	)
	if(DEFINED MFC_AGENT_SVC_URL)
		set(MFC_DEFINES ${MFC_DEFINES} MFC_AGENT_SVC_URL="${MFC_AGENT_SVC_URL}")
	endif()
	set(MFC_DEFINES_CEF
		MFC_CEF_API_DOLOGIN="${MFC_CEF_API_DOLOGIN}"
		MFC_CEF_API_MODEL_STREAM_KEY="${MFC_CEF_API_MODEL_STREAM_KEY}"
		MFC_CEF_API_OBJECT="${MFC_CEF_API_OBJECT}"
		MFC_CEF_APP_EXE_NAME="${MFC_CEF_APP_EXE_NAME}"
		MFC_CEF_LOGIN_URL="${MFC_CEF_LOGIN_URL}"
		MFC_OBS_CEF_LOGIN_BIN_PATH="${MFC_OBS_CEF_LOGIN_BIN_PATH}"
		MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD="${MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}"
	)
	set(MFC_DEFINES_Mac
	)
	set(MFC_DEFINES_Win
		MFC_OBS_BROADCAST_BIN_PATH="${MFC_OBS_PLUGIN_BIN_PATH}"
		MFC_OBS_BROADCAST_DATA_PATH="${MFC_OBS_PLUGIN_DATA_PATH}"
	)
	set(OTHER_DEFINES_Mac
	)
	set(OTHER_DEFINES_Win
		_HAS_STD_BYTE=0
	)
	set(COMPILE_OPTIONS_Mac
		-fexceptions
		-frtti
		-Wno-error
		-Wno-unused-parameter
	)
	set(COMPILE_OPTIONS_Win
		$<IF:$<CONFIG:Debug>,/MTd,/MT>
	)
	if(APPLE)
		target_compile_definitions(${TheTarget} PUBLIC
			${MFC_DEFINES}
			${MFC_DEFINES_CEF}
			${MFC_DEFINES_Mac}
			${OTHER_DEFINES_Mac}
		)
		target_compile_options(${TheTarget} PUBLIC
			${COMPILE_OPTIONS_Mac}
		)
		target_link_options(${TheTarget} PUBLIC
			# -Wl,-dead_strip
			# -Wl,-dead_strip_dylibs
		)
	elseif(WIN32)
		target_compile_definitions(${TheTarget} PUBLIC
			${MFC_DEFINES}
			${MFC_DEFINES_CEF}
			${MFC_DEFINES_Win}
			${OTHER_DEFINES_Win}
		)
		target_compile_options(${TheTarget} PUBLIC
			${COMPILE_OPTIONS_Win}
		)
	endif()
endfunction()

function(PRINT _dep)
	if(NOT ${${_dep}_FOUND})
		message(STATUS "${_dep} not found")
	endif()
	if(${_dep}_VERSION)
		message(STATUS "${_dep}_VERSION: ${${_dep}_VERSION}")
	endif()
	if(${_dep}_ROOT_DIR)
		message(STATUS "${_dep}_ROOT_DIR: ${${_dep}_ROOT_DIR}")
	endif()
	if(${_dep}_INCLUDE_DIR)
		message(STATUS "${_dep}_INCLUDE_DIR: ${${_dep}_INCLUDE_DIR}")
	endif()
	if(${_dep}_INCLUDE_DIRS)
		message(STATUS "${_dep}_INCLUDE_DIRS:")
		foreach(_dir "${${_dep}_INCLUDE_DIRS}")
			message(STATUS "-- ${_dir}")
		endforeach()
	endif()
	if(${_dep}_LIBRARY)
		message(STATUS "${_dep}_LIBRARY: ${${_dep}_LIBRARY}")
	endif()
	if(${_dep}_LIBRARY_DIR)
		message(STATUS "${_dep}_LIBRARY_DIR: ${${_dep}_LIBRARY_DIR}")
	endif()
	if(${_dep}_LIBRARIES)
		message(STATUS "${_dep}_LIBRARIES:")
		foreach(_lib "${${_dep}_LIBRARIES}")
			message(STATUS "-- ${_lib}")
		endforeach()
	endif()
endfunction()

# Apply patches to obs-studio
function(APPLY_PATCHES)
	file(GLOB PATCHES_ALL "${CMAKE_CURRENT_SOURCE_DIR}/patches/*.patch")
	file(GLOB PATCHES_MAC "${CMAKE_CURRENT_SOURCE_DIR}/patches/mac/*.patch")
	file(GLOB PATCHES_WIN "${CMAKE_CURRENT_SOURCE_DIR}/patches/win/*.patch")
	if(APPLE)
		set(PATCHES ${PATCHES_ALL} ${PATCHES_MAC})
	elseif(WIN32)
		set(PATCHES ${PATCHES_ALL} ${PATCHES_WIN})
	endif()
	if (PATCHES)
		set(PATCHES_NAMES "")
		foreach(PATCH ${PATCHES})
			get_filename_component(PATCH_NAME ${PATCH} NAME_WE)
			list(APPEND PATCHES_NAMES ${PATCH_NAME})
		endforeach()
		message(STATUS "Patches: ${PATCHES_NAMES}")
		execute_process(COMMAND git checkout -- cmake
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			OUTPUT_VARIABLE OUTPUT
			RESULT_VARIABLE RESULT
		)
		foreach(PATCH ${PATCHES})
			get_filename_component(PATCH_NAME ${PATCH} NAME_WE)
			message(STATUS "Applying patch ${PATCH_NAME}")
			execute_process(COMMAND git apply -p1 --ignore-whitespace --whitespace=nowarn
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
				INPUT_FILE "${PATCH}"
				OUTPUT_VARIABLE OUTPUT
				RESULT_VARIABLE RESULT
			)
			if (RESULT EQUAL 0)
				message(STATUS "Applying patch ${PATCH_NAME} - success")
			else()
				# Unfortunately although patch will recognise that a patch is already
				# applied it will still return an error.
				execute_process(COMMAND git apply -p1 -R --check --ignore-whitespace --whitespace=nowarn
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
					INPUT_FILE "${PATCH}"
					OUTPUT_VARIABLE OUTPUT_R
					RESULT_VARIABLE RESULT_R
				)
				if (RESULT_R EQUAL 0)
					message(STATUS "Applying patch ${PATCH_NAME} - already applied")
				else()
					message(STATUS "Applying patch ${PATCH_NAME} - error")
				endif()
			endif()
		endforeach()
	endif()
endfunction()

function(SETVAR _var)
	set(${_var} "${${_var}}" PARENT_SCOPE)
endfunction()

function(SETVARS _varlist)
	foreach(_var ${_varlist})
		set(${_var} "${${_var}}" PARENT_SCOPE)
	endforeach()
endfunction()

function(BUILD_INFO)
	# Get the build version (Sidekick)
	execute_process(
		COMMAND git describe --always --tags --abbrev=0 HEAD
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick"
		OUTPUT_VARIABLE SIDEKICK_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(REGEX MATCH "^[0-9]+\.[0-9]+\.[0-9]+" SIDEKICK_VER ${SIDEKICK_VERSION})
	string(REPLACE "." ";" SIDEKICK_VER_SPLIT ${SIDEKICK_VER})
	list(GET SIDEKICK_VER_SPLIT 0 SIDEKICK_VER_MAJOR)
	list(GET SIDEKICK_VER_SPLIT 1 SIDEKICK_VER_MINOR)
	list(GET SIDEKICK_VER_SPLIT 2 SIDEKICK_VER_BUILD)
	# Get the current working branch (Sidekick)
	execute_process(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick"
		OUTPUT_VARIABLE SIDEKICK_GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	# Get the latest abbreviated commit hash of the working branch (Sidekick)
	execute_process(
		COMMAND git log -1 --format=%h
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick"
		OUTPUT_VARIABLE SIDEKICK_GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	# Get the current working branch (OBS)
	execute_process(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE OBS_GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	# Get the latest abbreviated commit hash of the working branch (OBS)
	execute_process(
		COMMAND git log -1 --format=%h
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE OBS_GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(TIMESTAMP SIDEKICK_VERSION_BUILDTM "%Y-%m-%d %H:%M:%S")
	string(TIMESTAMP SIDEKICK_BUILD_DATE "%Y%m%d")
	string(TIMESTAMP SIDEKICK_BUILD_DATETIME "%Y%m%d-%H%M")
	string(TIMESTAMP SIDEKICK_BUILD_YEAR "%Y")
	set(SIDEKICK_VERSION_STR ${SIDEKICK_VERSION})
	set(SIDEKICK_WIN_VERSION_STR "${SIDEKICK_VER}.0")
	set(BUILD_VARS
		SIDEKICK_VER
		SIDEKICK_VERSION
		SIDEKICK_VERSION_STR
		SIDEKICK_WIN_VERSION_STR
		SIDEKICK_VER_MAJOR
		SIDEKICK_VER_MINOR
		SIDEKICK_VER_BUILD
		SIDEKICK_VERSION_BUILDTM
		SIDEKICK_BUILD_DATE
		SIDEKICK_BUILD_DATETIME
		SIDEKICK_BUILD_YEAR
		SIDEKICK_GIT_BRANCH
		SIDEKICK_GIT_COMMIT_HASH
		OBS_GIT_BRANCH
		OBS_GIT_COMMIT_HASH
	)
	foreach(_var ${BUILD_VARS})
		set(${_var} "${${_var}}" PARENT_SCOPE)
	endforeach()
	if(APPLE)
		configure_file(
			"${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick/scripts/install/osx/sidekick-obs26.pkgproj.in"
			"${CMAKE_BINARY_DIR}/sidekick.pkgproj"
		)
	elseif(WIN32)
		configure_file(
			"${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick/installer/Installer.iss.in"
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/installer/Installer.iss"
		)
		configure_file(
			"${CMAKE_SOURCE_DIR}/plugins/MyFreeCams/Sidekick/installer/Installer-del-obs-browser.iss.in"
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/installer/Installer-del-obs-browser.iss"
		)
	endif()
endfunction()