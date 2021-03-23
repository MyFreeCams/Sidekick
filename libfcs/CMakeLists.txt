#######################################
#  libfcs                             #
#  -(description)                     #
#######################################
#  Target: MFClibfcs                  #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(MyTarget "MFClibfcs")

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(SRC_LIBFCS
	Compat.h
	FcMsg.h
	fcs.h
	fcslib_string.h
	# fcslib_util.h
	fcs_b64.h
	fcs_b64.cpp
	FcUserIds.h
	gettimeofday.cpp
	ILog.h
	jsmin.h
	jsmin.cpp
	# json_ConvertUTF.h
	# json_ConvertUTF.cpp
	JSON_parser.h
	JSON_parser.cpp
	Log.h
	Log.cpp
	md5.h
	md5.cpp
	MfcJson.h
	MfcJson.cpp
	MfcLog.h
	MfcLog.cpp
	MfcTimer.h
	UtilCommon.h
	UtilCommon.cpp
	UtilString.h
	UtilString.cpp
)
set(SRC_LIBFCS_Win
)
set(SRC_LIBFCS_Mac
)

if(APPLE)
	add_library(${MyTarget} STATIC
		${SRC_LIBFCS}
		${SRC_LIBFCS_Mac}
	)
elseif(WIN32)
	add_library(${MyTarget} STATIC
		${SRC_LIBFCS}
		${SRC_LIBFCS_Win}
	)
endif()

if(APPLE)
	list(REMOVE_ITEM CMAKE_CXX_FLAGS -fno-exceptions -fno-rtti -Werror -funwind-tables -fno-asynchronous-unwind-tables)
	# Turn on RTTI.
	target_compile_options(${MyTarget} PUBLIC -fexceptions -frtti -Wno-unused-variable -Wno-unused-parameter -Wno-error)
elseif(WIN32)
	# Turn on RTTI. Disable warnings as errors.
	target_compile_options(${MyTarget} PUBLIC /GR /WX-)
endif()
