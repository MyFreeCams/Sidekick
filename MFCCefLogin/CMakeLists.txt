#######################################
#  MFCCefLogin                        #
#  -(description)                     #
#######################################
#  Target: MFCCefLogin                #
#  Target: MFCCefLoginHelper          #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

#
# Shared configuration.
#

# Target executable names.
set(CEF_TARGET "MFCCefLogin")

if(OS_MAC)
	set(CEF_HELPER_TARGET "MFCCefLoginHelper")
	set(CEF_HELPER_OUTPUT_NAME "MFCCefLogin Helper")
else()
	# Logical target used to link the libcef library.
	ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
endif()

set(CEF_BINARY_DIR $<IF:$<CONFIG:Debug>,${CEF_BINARY_DIR_DEBUG},${CEF_BINARY_DIR_RELEASE}>)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()

#
# Source files.
#

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/mfc_login.js.in
	${CMAKE_CURRENT_BINARY_DIR}/mfc_login.js
)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/MFCCefLogin.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/MFCCefLogin.rc
)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.in
	${CMAKE_CURRENT_BINARY_DIR}/mac/Info.plist
)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/mac/English.lproj/InfoPlist.strings.in
	${CMAKE_CURRENT_BINARY_DIR}/mac/English.lproj/InfoPlist.strings
)

# MFCCefLogin sources.
set(MFC_CEF_LOGIN_SRCS
	IPCWorkerThread.h
	IPCWorkerThread.cpp
	MFCCefEventHandler.h
	MFCCefEventHandler.cpp
	MFCJsExtensions.h
	MFCJsExtensions.cpp
)
set(MFC_CEF_LOGIN_SRCS_MAC
	MFCCefLogin_mac.mm
)
set(MFC_CEF_LOGIN_SRCS_WINDOWS
	MFCCefLogin_win.cpp
	MFCCefLogin.exe.manifest
	${CMAKE_CURRENT_BINARY_DIR}/MFCCefLogin.rc
	resource.h
)
APPEND_PLATFORM_SOURCES(MFC_CEF_LOGIN_SRCS)
source_group(MFCCefLogin FILES ${MFC_CEF_LOGIN_SRCS})

set(MFC_CEF_LOGIN_RES_SRCS_WINDOWS
	res/MFCCefLogin.ico
	res/small.ico
)
APPEND_PLATFORM_SOURCES(MFC_CEF_LOGIN_RES_SRCS)
source_group(MFCCefLogin\\\\res FILES ${MFC_CEF_LOGIN_RES_SRCS})

set(MFC_CEF_LOGIN_SRCS
	${MFC_CEF_LOGIN_SRCS}
	${MFC_CEF_LOGIN_RES_SRCS}
)

# MFCCefLogin helper sources.
set(MFC_CEF_LOGIN_HELPER_SRCS_MAC
	IPCWorkerThread.h
	IPCWorkerThread.cpp
	MFCCefEventHandler.h
	MFCCefEventHandler.cpp
	MFCJSExtensions.h
	MFCJSExtensions.cpp
	process_helper_mac.cc
)
APPEND_PLATFORM_SOURCES(MFC_CEF_LOGIN_HELPER_SRCS)
source_group(MFCCefLogin FILES ${MFC_CEF_LOGIN_HELPER_SRCS})

# MFCCefLogin resources.
set(MFC_CEF_LOGIN_RESOURCES_MAC_SRCS_MAC
	${CMAKE_CURRENT_BINARY_DIR}/mac/Info.plist
	mac/MFCCefLogin.icns
)
APPEND_PLATFORM_SOURCES(MFC_CEF_LOGIN_RESOURCES_MAC_SRCS)
source_group(MFCCefLogin\\\\mac_src FILES ${MFC_CEF_LOGIN_RESOURCES_MAC_SRCS})

set(MFC_CEF_LOGIN_RESOURCES_MAC_ENGLISH_LPROJ_SRCS_MAC
	${CMAKE_CURRENT_BINARY_DIR}/mac/English.lproj/InfoPlist.strings
	mac/English.lproj/MainMenu.xib
)
APPEND_PLATFORM_SOURCES(MFC_CEF_LOGIN_RESOURCES_MAC_ENGLISH_LPROJ_SRCS)
source_group(MFCCefLogin\\\\mac\\\\English.lproj FILES ${MFC_CEF_LOGIN_RESOURCES_MAC_ENGLISH_LPROJ_SRCS})

set(MFC_CEF_LOGIN_RESOURCES_SRCS
	${MFC_CEF_LOGIN_RESOURCES_MAC_SRCS}
	${MFC_CEF_LOGIN_RESOURCES_MAC_ENGLISH_LPROJ_SRCS}
)

#
# macOS configuration.
#
if(OS_MAC)
	option(OPTION_USE_ARC "Build with ARC (automatic Reference Counting) on macOS." ON)
	if(OPTION_USE_ARC)
		message(STATUS "ARC (automatic reference counting) enabled")
		list(APPEND CEF_COMPILER_FLAGS -fobjc-arc)
		set_target_properties(${target} PROPERTIES CLANG_ENABLE_OBJC_ARC "YES")
	endif()

	# Output paths for the app bundles.
	set(CEF_APP "${CEF_TARGET_OUT_DIR}/${CEF_TARGET}.app")
	set(CEF_HELPER_APP "${CEF_TARGET_OUT_DIR}/${CEF_HELPER_OUTPUT_NAME}.app")

	message(STATUS "CEF_APP: ${CEF_APP}")
	message(STATUS "CEF_HELPER_APP: ${CEF_HELPER_APP}")

	# Variables referenced from the main Info.plist file.
	set(PRODUCT_NAME "${CEF_TARGET}")
	set(PRODUCT_VERSION ${SIDEKICK_VERSION_STR})

	# Helper executable target.
	add_executable(${CEF_HELPER_TARGET} MACOSX_BUNDLE ${MFC_CEF_LOGIN_HELPER_SRCS})
	SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_HELPER_TARGET})
	#add_dependencies(${CEF_HELPER_TARGET} libcef_dll_wrapper)
	target_link_libraries(${CEF_HELPER_TARGET}
		libcef_dll_wrapper
		${CEF_STANDARD_LIBS}
		MFCLibCef
		${Boost_LIBRARIES}
		libipc
	)
	set_target_properties(${CEF_HELPER_TARGET} PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/helper-Info.plist
		OUTPUT_NAME ${CEF_HELPER_OUTPUT_NAME}
	)
	target_compile_definitions(${CEF_HELPER_TARGET}
		PUBLIC MFC_LOG_LEVEL=${MFC_LOG_LEVEL}
		PUBLIC MFC_LOG_OUTPUT_MASK=${MFC_LOG_OUTPUT_MASK}
	)
	MFCDefines(${CEF_HELPER_TARGET})

	if(USE_SANDBOX)
		ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
		target_link_libraries(${CEF_HELPER_TARGET} cef_sandbox_lib)
	endif()

	# Main executable target.
	add_executable(${CEF_TARGET} MACOSX_BUNDLE
		${MFC_CEF_LOGIN_RESOURCES_SRCS}
		${MFC_CEF_LOGIN_SRCS}
	)
	SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
	add_dependencies(${CEF_TARGET}
		#libcef_dll_wrapper
		${CEF_HELPER_TARGET}
		#MFCLibCef
	)
	target_link_libraries(${CEF_TARGET}
		libcef_dll_wrapper
		${CEF_STANDARD_LIBS}
		MFCLibCef
		${Boost_LIBRARIES}
		libipc
	)
	target_include_directories(${CEF_TARGET} PUBLIC
			../libfcs
			../libipc
			)
	target_include_directories(${CEF_HELPER_TARGET} PUBLIC
			../libfcs
			../libipc
			)

	set_target_properties(${CEF_TARGET} PROPERTIES
		MACOSX_BUNDLE TRUE
		# MACOSX_BUNDLE_BUNDLE_NAME ${CEF_TARGET}
		# MACOSX_BUNDLE_BUNDLE_VERSION ${PRODUCT_VERSION}
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/mac/Info.plist"
		# MACOSX_BUNDLE_INFO_STRING "${CEF_TARGET} v${PRODUCT_VERSION}"
		# MACOSX_BUNDLE_SHORT_VERSION_STRING ${PRODUCT_VERSION}
		# MACOSX_BUNDLE_GUI_IDENTIFIER "com.myfreecams.sidekick-login"
		# MACOSX_BUNDLE_COPYRIGHT "(C) 2020 MyFreeCams"
		MACOSX_RPATH TRUE
		# OUTPUT_NAME ${CEF_TARGET}
		RESOURCE "${MFC_CEF_LOGIN_RESOURCES_SRCS}"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
		# XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME TRUE
		# XCODE_ATTRIBUTE_EXECUTABLE_NAME ${CEF_TARGET}
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
	)
	MFCDefines(${CEF_TARGET})

	# Manually process and copy over resource files.
	# The Xcode generator can support this via the set_target_properties RESOURCE
	# directive but that doesn't properly handle nested resource directories.
	# Remove these prefixes from input file paths.
	set(PREFIXES "mac/")
	COPY_MAC_RESOURCES(
		"${MFC_CEF_LOGIN_RESOURCES_SRCS}"
		"${PREFIXES}"
		"${CEF_TARGET}"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CEF_APP}"
	)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
	target_compile_options(${CEF_TARGET} PUBLIC -fexceptions -Wno-error -Wno-unused-parameter -Wno-unused-variable -fvisibility=hidden -fvisibility-inlines-hidden)
	target_compile_options(${CEF_HELPER_TARGET} PUBLIC -fexceptions -Wno-error -Wno-unused-parameter -Wno-unused-variable -fvisibility=hidden -fvisibility-inlines-hidden)
endif()

#
# Windows configuration.
#
if(OS_WINDOWS)
	# Executable target.
	add_executable(${CEF_TARGET} WIN32 ${MFC_CEF_LOGIN_SRCS})
	#add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
	SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
	target_link_directories(${CEF_TARGET} PUBLIC ${Boost_LIBRARY_DIRS})
	target_link_libraries(${CEF_TARGET}
		libcef_lib
		libcef_dll_wrapper
		${CEF_STANDARD_LIBS}
		MFCLibCef
		${Boost_LIBRARIES}
		libipc
	)
	# Turn on RTTI & disable warnings as errors
	target_compile_options(${CEF_TARGET} PUBLIC /MT /GR /WX-)
	MFCDefines(${CEF_TARGET})

	if(USE_SANDBOX)
		ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
		target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
	endif()

	# Add the custom manifest files to the executable.
	ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_TARGET}" "exe")

	# Copy binary and resource files to the target output directory.
	COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
	COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
endif()

message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

#
# Post Build.
#
if(OS_MAC)
	# Copy boost libs into temp directory
	# file(MAKE_DIRECTORY boost)
	# foreach(_lib ${Boost_LIBRARIES})
	#   file(COPY ${_lib} DESTINATION "boost/"
	#       FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ
	#   )
	# endforeach()

	# Fix install names of boost libs
	# foreach(lib ${Boost_LIBRARIES})
	#   get_filename_component(lib_name ${lib} NAME)
	#   add_custom_command(TARGET ${CEF_TARGET}
	#       POST_BUILD
	#       COMMAND ${CMAKE_INSTALL_NAME_TOOL} -id
	#           @executable_path/../Frameworks/${lib_name}
	#           "boost/${lib_name}"
	#       COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change ${lib}
	#           @executable_path/../Frameworks/${lib_name}
	#           "${CEF_APP}/Contents/MacOS/MFCCefLogin"
	#   )
	# endforeach()

	# Copy dependencies into the main app bundle.
	add_custom_command(TARGET ${CEF_TARGET}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E echo "** Building Sidekick Login app bundle **"
		# Copy the helper app bundle into the Frameworks subdirectory.
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_HELPER_APP}"
			"${CEF_APP}/Contents/Frameworks/${CEF_HELPER_OUTPUT_NAME}.app"

		COMMAND ${CMAKE_COMMAND} -E echo "copy libipc ${CMAKE_CURRENT_BINARY_DIR}/../libipc/libipc.dylib ${CEF_APP}/Contents/Frameworks/${CEF_HELPER_OUTPUT_NAME}.app/Contents/MacOS/libipc.dylib"

			COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/../libipc/libipc.dylib"
			"${CEF_APP}/Contents/Frameworks/${CEF_HELPER_OUTPUT_NAME}.app/Contents/MacOS/libipc.dylib"

		COMMAND ${CMAKE_COMMAND} -E echo "PATH: ${CMAKE_CURRENT_BINARY_DIR}/../libipc/libipc.dylib ${CEF_APP}/Contents/macos/libipc.dylib"

		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/../libipc/libipc.dylib"
			"${CEF_APP}/Contents/macos/libipc.dylib"
		# Copy the CEF framework into the Frameworks subdirectory.
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CEF_BINARY_DIR}/Chromium Embedded Framework.framework"
			"${CEF_APP}/Contents/Frameworks/Chromium Embedded Framework.framework"
		# Copy boost libs into the Frameworks subdirectory
		# COMMAND ${CMAKE_COMMAND} -E copy_directory "boost/" "${CEF_APP}/Contents/Frameworks/"
		VERBATIM
	)

	# Copy main app bundle to destination
	add_custom_command(TARGET ${CEF_TARGET}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${MFC_OBS_CEF_LOGIN_BIN_PATH}"
		# Install MFCCefLogin.app in /Library/Application Support/obs-studio/sidekick
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_APP}"
			"${MFC_OBS_CEF_LOGIN_BIN_PATH}/${CEF_TARGET}.app"
		# Copy to configuration independent location for packaging script
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_APP}"
			"${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/MFCCefLogin/${CEF_TARGET}.app"
	)
endif()

if(OS_WINDOWS)
	set(OUTFILE $<TARGET_FILE:${CEF_TARGET}>)
	file(TO_NATIVE_PATH ${OUTFILE} NATIVE_TARGET_FILE)
	file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/mfc_login.js NATIVE_MFC_LOGIN_JS)
	file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG> NATIVE_CEF_DIRECTORY_PATH)

	# ${CMAKE_BINARY_DIR}/Sidekick/cef
	file(TO_NATIVE_PATH ${MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD} NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD)
	# C:\Users\Public\Sidekick\cef
	file(TO_NATIVE_PATH ${MFC_OBS_CEF_LOGIN_BIN_PATH} NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH)

	# Create target build & install directories
	add_custom_command(TARGET ${CEF_TARGET}
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}
	)

	# Copy target to build & install paths
	message(STATUS "Copying ${NATIVE_TARGET_FILE} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}")
	message(STATUS "Copying ${NATIVE_TARGET_FILE} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}")
	add_custom_command(TARGET ${CEF_TARGET}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_FILE} ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}
		COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_FILE} ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}
	)

	# Copy cef directory to build & install paths
	message(STATUS "Copying ${NATIVE_CEF_DIRECTORY_PATH} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}")
	message(STATUS "Copying ${NATIVE_CEF_DIRECTORY_PATH} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}")
	add_custom_command(TARGET ${CEF_TARGET}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${NATIVE_CEF_DIRECTORY_PATH}
			${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${NATIVE_CEF_DIRECTORY_PATH}
			${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}
	)

	message(STATUS "Copying ${NATIVE_MFC_LOGIN_JS} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD}")
	file(COPY ${NATIVE_MFC_LOGIN_JS} DESTINATION ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD})
	message(STATUS "Copying ${NATIVE_MFC_LOGIN_JS} => ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH}")
	file(COPY ${NATIVE_MFC_LOGIN_JS} DESTINATION ${NATIVE_MFC_OBS_CEF_LOGIN_BIN_PATH})
endif()
