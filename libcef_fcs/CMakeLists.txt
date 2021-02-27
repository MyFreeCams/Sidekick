#######################################
#  libcef_fcs                         #
#  -(description)                     #
#######################################
#  Target: MFCLibCef                  #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(CEF_TARGET MFCLibCef)

#
# Source files.
#
set(LIBCEF_FCS_SRCS
	../libfcs/Compat.h
	../libfcs/fcs_b64.h
	../libfcs/fcs_b64.cpp
	../libfcs/gettimeofday.cpp
	../libfcs/ILog.h
	../libfcs/jsmin.h
	../libfcs/jsmin.cpp
	../libfcs/JSON_parser.h
	../libfcs/JSON_parser.cpp
	../libfcs/Log.h
	../libfcs/Log.cpp
	../libfcs/md5.h
	../libfcs/md5.cpp
	../libfcs/MfcJson.h
	../libfcs/MfcJson.cpp
	../libfcs/MfcLog.h
	../libfcs/MfcLog.cpp
	../libfcs/MfcTimer.h
	../libfcs/UtilCommon.h
	../libfcs/UtilCommon.cpp
	../libfcs/UtilString.h
	../libfcs/UtilString.cpp
)
source_group(libfcs FILES ${LIBCEF_FCS_SRCS})

set(LIBCEF_FCS_CEF_SRCS
	cefEventHandler.h
	cefEventHandler.cpp
	cefJSExtensions.h
	cefJSExtensions.cpp
	cefLogin_app.h
)
# source_group(libcef_fcs\\cef FILES ${LIBCEF_FCS_CEF_SRCS})

set(OBS_CEF_LOGINSRCS_MAC
	CefEventHandler_mac.mm
)
# source_group(libcef_fcs\\cef\\mac FILES ${OBS_CEF_LOGINSRCS_MAC})

set(OBS_CEF_LOGINSRCS_WINDOWS
	CefLogin_handler_win.cpp
)
# source_group(libcef_fcs\\cef\\win FILES ${OBS_CEF_LOGINSRCS_WINDOWS})

if(APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	add_library(${CEF_TARGET}
		${LIBCEF_FCS_SRCS}
		${LIBCEF_FCS_CEF_SRCS}
		${OBS_CEF_LOGINSRCS_MAC}
	)
elseif(WIN32)
	add_library(${CEF_TARGET}
		${LIBCEF_FCS_SRCS}
		${LIBCEF_FCS_CEF_SRCS}
		${OBS_CEF_LOGINSRCS_WINDOWS}
	)
endif()
SET_LIBRARY_TARGET_PROPERTIES(${CEF_TARGET})

if(APPLE)
	list(REMOVE_ITEM CEF_CXX_COMPILER_FLAGS -fno-exceptions -fno-rtti -funwind-tables -fno-asynchronous-unwind-tables)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -funwind-tables -fno-asynchronous-unwind-tables)
	# Turn on RTTI.
	target_compile_options(${CEF_TARGET} PUBLIC -fexceptions -frtti -Wno-unused-variable -Wno-unused-parameter -Wno-error)
	target_link_libraries(${CEF_TARGET} PUBLIC ${COCOA_LIBRARY})
elseif(WIN32)
	# Turn on RTTI. Disable warnings as errors.
	target_compile_options(${CEF_TARGET} PUBLIC /GR /WX-)
endif()

# Remove the default "lib" prefix from the resulting library.
set_target_properties(${CEF_TARGET} PROPERTIES PREFIX "")

target_compile_definitions(${CEF_TARGET} PUBLIC
	# BOOST_ALL_NO_LIB
)

MFCDefines(${CEF_TARGET})
