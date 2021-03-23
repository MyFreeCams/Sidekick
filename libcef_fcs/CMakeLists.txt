#######################################
#  libcef_fcs                         #
#  -(description)                     #
#######################################
#  Target: MFCLibCef                  #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(CEF_TARGET "MFCLibCef")

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#
# Source files.
#
set(LIBCEF_FCS_SRCS
	../libfcs/Compat.h
	# ../libfcs/FcMsg.h
	../libfcs/fcs.h
	../libfcs/fcs_b64.h
	../libfcs/fcs_b64.cpp
	../libfcs/fcslib_string.h
	../libfcs/fcslib_util.h
	../libfcs/FcUserIds.h
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
set(LIBCEF_FCS_CEF_SRCS_MAC
	CefEventHandler_mac.mm
)
set(LIBCEF_FCS_CEF_SRCS_WINDOWS
	CefLogin_handler_win.cpp
)
APPEND_PLATFORM_SOURCES(LIBCEF_FCS_CEF_SRCS)
source_group(libcef_fcs FILES ${LIBCEF_FCS_CEF_SRCS})

#
# Dependencies
#

#------------------------------------------------------------------------
# Boost
#
if(NOT DEFINED Boost::headers)
	set(Boost_USE_DEBUG_LIBS OFF)
	set(Boost_USE_RELEASE_LIBS ON)
	#set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_DEBUG_RUNTIME OFF)
	#set(BOOST_NO_EXCEPTIONS ON)
	find_package(Boost 1.75 COMPONENTS date_time thread system REQUIRED)
	PRINT(Boost)
endif()

# create library target
add_library(${CEF_TARGET}
	${LIBCEF_FCS_SRCS}
	${LIBCEF_FCS_CEF_SRCS}
)

target_link_libraries(${CEF_TARGET} PRIVATE Boost::headers)

SET_LIBRARY_TARGET_PROPERTIES(${CEF_TARGET})
# Remove the default "lib" prefix from the resulting library.
set_target_properties(${CEF_TARGET} PROPERTIES PREFIX "")

MFCDefines(${CEF_TARGET})

if(APPLE)
	list(REMOVE_ITEM CEF_CXX_COMPILER_FLAGS -fno-exceptions -fno-rtti -funwind-tables -fno-asynchronous-unwind-tables)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -funwind-tables -fno-asynchronous-unwind-tables)
	# Turn on RTTI.
	target_compile_options(${CEF_TARGET} PUBLIC -fexceptions -frtti -Wno-unused-variable -Wno-unused-parameter -Wno-error)
elseif(WIN32)
	# Turn on RTTI. Disable warnings as errors.
	target_compile_options(${CEF_TARGET} PUBLIC /GR /WX-)
endif()

# target_compile_definitions(${CEF_TARGET} PUBLIC BOOST_ALL_NO_LIB)
