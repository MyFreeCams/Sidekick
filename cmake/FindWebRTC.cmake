#[=======================================================================[.rst:

FindWebRTC
----------

Find the WebRTC library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following imported target:

``WebRTC::WebRTC``


Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``WEBRTC_FOUND``
``WEBRTC_INCLUDE_DIRS``
``WEBRTC_LIBRARIES``
``WEBRTC_ROOT_DIR``
``WEBRTC_VERSION``


Hints
^^^^^

Set ``WEBRTC_ROOT_DIR`` to the root directory of a WebRTC installation.

#]=======================================================================]

if(WEBRTC_CMAKE_DIR)
	set(libwebrtc_DIR WEBRTC_CMAKE_DIR)
endif()
if(LIBWEBRTC_CMAKE_DIR)
	set(libwebrtc_DIR LIBWEBRTC_CMAKE_DIR)
endif()

if(libwebrtc_DIR)
	set(WEBRTC_CMAKE_DIR ${libwebrtc_DIR})
	list(APPEND CMAKE_MODULE_PATH ${WEBRTC_CMAKE_DIR})
	list(APPEND CMAKE_PREFIX_PATH ${WEBRTC_CMAKE_DIR})
	add_definitions(-DWEBRTC_LIBRARY_IMPL -DABSL_ALLOCATOR_NOTHROW=1)
	if(APPLE)
		add_definitions(-DWEBRTC_POSIX -DWEBRTC_MAC -DHAVE_PTHREAD)
	elseif(WIN32)
		add_definitions(-DWEBRTC_WIN -DWIN32 -D_WINDOWS -D__STD_C
			-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_UNICODE -DUNICODE)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		add_definitions(-DWEBRTC_LINUX -D_GLIBCXX_USE_CXX11_ABI=0)
	endif()
	find_package(libwebrtc CONFIG REQUIRED)
	return()
endif()

set(WEBRTC_ROOT_DIR "" CACHE STRING "Where is the WebRTC root directory located?")

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_WebRTC QUIET webrtc libwebrtc webrtc_full libwebrtc_full)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(WEBRTC_INCLUDE_DIR
	NAMES
		api/peer_connection_interface.h
		api/peerconnectioninterface.h
	HINTS
		${WEBRTC_ROOT_DIR}
		${WEBRTC_ROOT}
		${WEBRTC}
		ENV WEBRTC_ROOT_DIR
		ENV WEBRTC_ROOT
		ENV WEBRTC
		${LIBWEBRTC_ROOT_DIR}
		${LIBWEBRTC_ROOT}
		${LIBWEBRTC}
		ENV LIBWEBRTC_ROOT_DIR
		ENV LIBWEBRTC_ROOT
		ENV LIBWEBRTC
		${PC_WebRTC_INCLUDE_DIRS}
	PATH_SUFFIXES
		include
		src
)

find_library(WEBRTC_LIB
	NAMES
		${PC_WebRTC_LIBRARIES}
		webrtc
		libwebrtc
		webrtc_full
		libwebrtc_full
	HINTS
		${WEBRTC_ROOT_DIR}
		${WEBRTC_ROOT}
		${WEBRTC}
		ENV WEBRTC_ROOT_DIR
		ENV WEBRTC_ROOT
		ENV WEBRTC
		${LIBWEBRTC_ROOT_DIR}
		${LIBWEBRTC_ROOT}
		${LIBWEBRTC}
		ENV LIBWEBRTC_ROOT_DIR
		ENV LIBWEBRTC_ROOT
		ENV LIBWEBRTC
		${PC_WebRTC_LIBRARY_DIRS}
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin
		src/out/Default/obj
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebRTC DEFAULT_MSG WEBRTC_LIB WEBRTC_INCLUDE_DIR)

if(WEBRTC_FOUND)
	get_filename_component(WEBRTC_ROOT_DIR ${WEBRTC_INCLUDE_DIR} DIRECTORY)
	file(STRINGS "${WEBRTC_ROOT_DIR}/webrtc_version.h"
		WEBRTC_VERSION_STRING
		REGEX "^[ \t]*#define[ \t]+WEBRTC_VERSION[ \t]+[0-9]+.*$"
	)
	if(WEBRTC_VERSION_STRING)
		string(REGEX REPLACE
			".*#define[ \t]+WEBRTC_VERSION[ \t]+([0-9]+).*"
			"\\1"
			WEBRTC_VERSION
			${WEBRTC_VERSION_STRING}
		)
	else()
		message(STATUS "webrtc_version.h not found")
		if(DEFINED $ENV{WEBRTC_VERSION})
			set(WEBRTC_VERSION "$ENV{WEBRTC_VERSION}")
		endif()
	endif()
else()
	message(STATUS "WebRTC not found")
endif()

#------------------------------------------------------------------------
# Platform dependencies & preprocessor definitions
#
if(APPLE)
	set(WEBRTC_PLATFORM_DEFINITIONS_RELEASE
		_LIBCPP_HAS_NO_ALIGNED_ALLOCATION
		CR_XCODE_VERSION=1130
		__STDC_CONSTANT_MACROS
		__STDC_FORMAT_MACROS
		_FORTIFY_SOURCE=2
		_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
		_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
		_LIBCPP_ENABLE_NODISCARD
		CR_LIBCXX_REVISION=375504
		__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0
		NDEBUG
		NVALGRIND
		DYNAMIC_ANNOTATIONS_ENABLED=0
		WEBRTC_ENABLE_PROTOBUF=1
		WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
		HAVE_SCTP
		WEBRTC_USE_H264
		WEBRTC_EXCLUDE_TRANSIENT_SUPPRESSOR
		WEBRTC_LIBRARY_IMPL
		WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0
		WEBRTC_POSIX
		WEBRTC_MAC
		ABSL_ALLOCATOR_NOTHROW=1
		HAVE_SCTP
		GOOGLE_PROTOBUF_NO_RTTI
		GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
		HAVE_PTHREAD
	) # m86
	set(WEBRTC_PLATFORM_DEFINITIONS_DEBUG
		_LIBCPP_HAS_NO_ALIGNED_ALLOCATION
		CR_XCODE_VERSION=1130
		__STDC_CONSTANT_MACROS
		__STDC_FORMAT_MACROS
		_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
		_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
		_LIBCPP_ENABLE_NODISCARD
		CR_LIBCXX_REVISION=375504
		__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0
		_DEBUG
		DYNAMIC_ANNOTATIONS_ENABLED=1
		WEBRTC_ENABLE_PROTOBUF=1
		WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
		RTC_ENABLE_VP9
		HAVE_SCTP
		WEBRTC_USE_H264
		WEBRTC_LIBRARY_IMPL
		WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=1
		WEBRTC_POSIX
		WEBRTC_MAC
		ABSL_ALLOCATOR_NOTHROW=1
		HAVE_SCTP
		GOOGLE_PROTOBUF_NO_RTTI
		GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
		HAVE_PTHREAD
	) #m80

	set(WEBRTC_PLATFORM_C_FLAGS
		-arch x86_64
		-fcolor-diagnostics
		-fcomplete-member-pointers
		-fmerge-all-constants
		-fno-omit-frame-pointer
		-fno-strict-aliasing
		-ftrivial-auto-var-init=pattern
		-fvisibility=hidden
		-g2
		-gdwarf-4
		-instcombine-lower-dbg-declare=0
		-mllvm
		-mmacosx-version-min=10.10.0
		-no-canonical-prefixes
		-std=c11
		# -Wall
		-Wc++11-narrowing
		# -Werror
		-Wexit-time-destructors
		# -Wextra
		-Wextra-semi
		-Wglobal-constructors
		-Wheader-hygiene
		-Wimplicit-fallthrough
		-Winconsistent-missing-override
		-Wno-builtin-assume-aligned-alignment
		# -Wno-builtin-macro-redefined
		-Wno-c++11-narrowing
		-Wno-c99-designator
		-Wno-deprecated-copy
		-Wno-final-dtor-non-final-class
		-Wno-ignored-pragma-optimize
		-Wno-implicit-int-float-conversion
		-Wno-max-tokens
		-Wno-missing-field-initializers
		-Wno-non-c-typedef-for-linkage
		-Wno-pointer-to-int-cast
		-Wno-psabi
		-Wno-reorder-init-list
		-Wno-shorten-64-to-32
		-Wno-sizeof-array-div
		-Wno-string-concatenation
		-Wno-undefined-var-template
		-Wno-unneeded-internal-declaration
		-Wno-unused-parameter
		-Wstrict-prototypes
		-Wstring-conversion
		-Wtautological-overlap-compare
		-Wthread-safety
		-Wundef
		-Wunguarded-availability
		-Wunreachable-code
		-Wunused-lambda-capture
	)
	set(WEBRTC_PLATFORM_C_FLAGS_RELEASE
		-O2
		-fno-standalone-debug
		-fstack-protector
	)
	set(WEBRTC_PLATFORM_C_FLAGS_DEBUG
		-O0
		-fstack-protector-strong
		-Wno-builtin-macro-redefined
	)

	set(WEBRTC_PLATFORM_CXX_FLAGS
		-fno-exceptions
		-fno-trigraphs
		-fvisibility-inlines-hidden
		-fobjc-arc
		-fobjc-call-cxx-cdtors
		-isystem ${WEBRTC_INCLUDE_DIR}/buildtools/third_party/libc++/trunk/include
		-isystem ${WEBRTC_INCLUDE_DIR}/buildtools/third_party/libc++abi/trunk/include
		-nostdinc++
		-std=c++14
		-stdlib=libc++
		-Wno-trigraphs
		-Wnon-virtual-dtor
		-Woverloaded-virtual
		-Wobjc-missing-property-synthesis
	)

	set(WEBRTC_PLATFORM_LINKER_FLAGS
		-Wl,-no_data_in_code_info
		-Wl,-dead_strip
		-Wl,-dead_strip_dylibs
		-Wl,-fatal_warnings
		-Wl,-no_function_starts
	)

	find_library(APP_KIT AppKit)
	find_library(APPLICATION_SERVICES ApplicationServices)
	find_library(AUDIO_TOOLBOX AudioToolbox)
	find_library(AV_FOUNDATION AVFoundation)
	find_library(CORE_AUDIO CoreAudio)
	find_library(CORE_GRAPHICS CoreGraphics)
	find_library(CORE_MEDIA CoreMedia)
	find_library(FOUNDATION Foundation)
	find_library(IO_KIT IOKit)
	find_library(IO_SURFACE IOSurface)

	set(WEBRTC_PLATFORM_DEPS
		${APP_KIT}
		${APPLICATION_SERVICES}
		${AUDIO_TOOLBOX}
		${AV_FOUNDATION}
		${CORE_AUDIO}
		${CORE_GRAPHICS}
		${CORE_MEDIA}
		${FOUNDATION}
		${IO_KIT}
		${IO_SURFACE}
	)
elseif(UNIX)
	set(WEBRTC_PLATFORM_DEFINITIONS_$<CONFIG> WEBRTC_POSIX)

	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		list(APPEND WEBRTC_PLATFORM_DEFINITIONS_$<CONFIG> WEBRTC_LINUX _GLIBCXX_USE_CXX11_ABI=0)
		set(WEBRTC_PLATFORM_DEPS
			-lrt
			-lX11
			-lGLU
			# -lGL
		)
	endif()
endif()

if(WIN32 AND MSVC)
	set(WEBRTC_PLATFORM_DEFINITIONS_RELEASE
		USE_AURA=1
		_HAS_NODISCARD
		_HAS_EXCEPTIONS=0
		__STD_C
		_CRT_RAND_S
		_CRT_SECURE_NO_DEPRECATE
		_SCL_SECURE_NO_DEPRECATE
		_ATL_NO_OPENGL
		_WINDOWS
		CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
		PSAPI_VERSION=2
		WIN32
		_SECURE_ATL
		_USING_V110_SDK71_
		WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP
		WIN32_LEAN_AND_MEAN
		NOMINMAX
		_UNICODE
		UNICODE
		NTDDI_VERSION=NTDDI_WIN10_RS2
		_WIN32_WINNT=0x0A00
		WINVER=0x0A00
		NDEBUG
		NVALGRIND
		DYNAMIC_ANNOTATIONS_ENABLED=0
		WEBRTC_ENABLE_PROTOBUF=1
		WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
		RTC_ENABLE_VP9
		HAVE_SCTP
		WEBRTC_USE_H264
		WEBRTC_LIBRARY_IMPL
		WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0
		WEBRTC_WIN
		ABSL_ALLOCATOR_NOTHROW=1
		HAVE_SCTP
		GOOGLE_PROTOBUF_NO_RTTI
		GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
	) # m80
	set(WEBRTC_PLATFORM_DEFINITIONS_DEBUG
		USE_AURA=1
		_HAS_NODISCARD
		_HAS_EXCEPTIONS=0
		__STD_C
		_CRT_RAND_S
		_CRT_SECURE_NO_DEPRECATE
		_SCL_SECURE_NO_DEPRECATE
		_ATL_NO_OPENGL
		_WINDOWS
		CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
		PSAPI_VERSION=2
		WIN32
		_SECURE_ATL
		_USING_V110_SDK71_
		WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP
		WIN32_LEAN_AND_MEAN
		NOMINMAX
		_UNICODE
		UNICODE
		NTDDI_VERSION=NTDDI_WIN10_RS2
		_WIN32_WINNT=0x0A00
		WINVER=0x0A00
		_DEBUG
		DYNAMIC_ANNOTATIONS_ENABLED=1
		WTF_USE_DYNAMIC_ANNOTATIONS=1
		_HAS_ITERATOR_DEBUGGING=0
		WEBRTC_ENABLE_PROTOBUF=1
		WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
		RTC_ENABLE_VP9
		HAVE_SCTP
		WEBRTC_USE_H264
		WEBRTC_LIBRARY_IMPL
		WEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0
		WEBRTC_WIN
		ABSL_ALLOCATOR_NOTHROW=1
		HAVE_SCTP
		GOOGLE_PROTOBUF_NO_RTTI
		GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
	) # m80

	set(WEBRTC_PLATFORM_C_FLAGS
		/bigobj
		/Brepro
		/Gy
		/wd4091 /wd4100 /wd4121 /wd4127 /wd4200 /wd4201
		/wd4204 /wd4221 /wd4244 /wd4245 /wd4251 /wd4267
		/wd4275 /wd4305 /wd4312 /wd4324 /wd4351 /wd4355
		/wd4389 /wd4456 /wd4457 /wd4458 /wd4459 /wd4503
		/wd4505 /wd4510 /wd4512 /wd4589 /wd4610 /wd4611
		/wd4661 /wd4701 /wd4702 /wd4703 /wd4706 /wd4715
		/wd4838 /wd4995 /wd4996
		/WX-
		/Zc:sizedDealloc-
		## -fcomplete-member-pointers
		## -fmerge-all-constants
		## -fno-standalone-debug
		## -gcodeview-ghash
		## -m64
		## -no-canonical-prefixes
		## -Wc++11-narrowing
		## -Wexit-time-destructors
		## -Wextra-semi
		## -Wglobal-constructors
		## -Wheader-hygiene
		## -Wimplicit-fallthrough
		## -Winconsistent-missing-override
		## -Wno-c++11-narrowing
		## -Wno-ignored-pragma-optimize
		## -Wno-missing-field-initializers
		## -Wno-nonportable-include-path
		## -Wno-shorten-64-to-32
		## -Wno-undefined-var-template
		## -Wno-unneeded-internal-declaration
		## -Wno-unused-parameter
		## -Wstring-conversion
		## -Wtautological-overlap-compare
		## -Wunused-lambda-capture
		## -Wthread-safety
		## -Wundef
	)
	set(WEBRTC_PLATFORM_C_FLAGS_RELEASE
		/MT
		/O1
		/Ob2
		/Oi
		/Oy-
		/Zc:inline
	)
	set(WEBRTC_PLATFORM_C_FLAGS_DEBUG
		/GF
		/MTd
		/Ob0
		/Od
		## -fcolor-diagnostics
		## -Wno-builtin-macro-redefined
	)

	set(WEBRTC_PLATFORM_CXX_FLAGS
		/wd4577
	)

	set(WEBRTC_PLATFORM_LINKER_FLAGS
		# /NATVIS:"${WEBRTC_ROOT_DIR}/src/build/config/c++/libc++.natvis"
		# /INCREMENTAL:NO
		# /FIXED:NO
		# /PROFILE
	)

	set(WEBRTC_PLATFORM_DEPS
		amstrmid.lib
		crypt32.lib
		d3d11.lib
		dmoguids.lib
		dxgi.lib
		iphlpapi.lib
		msdmo.lib
		secur32.lib
		strmiids.lib
		winmm.lib
		wmcodecdspuuid.lib
	)
endif()

if(WEBRTC_FOUND)
	set(WEBRTC_INCLUDE_DIR "${WEBRTC_ROOT_DIR}/src")
	set(WEBRTC_INCLUDE_DIRS
		"${WEBRTC_INCLUDE_DIR}"
		"${WEBRTC_INCLUDE_DIR}/third_party/abseil-cpp"
		"${WEBRTC_INCLUDE_DIR}/third_party/protobuf/src"
		"${WEBRTC_INCLUDE_DIR}/third_party/libyuv/include"
		"${WEBRTC_PLATFORM_INCLUDE_DIRS}"
	)

	set(WEBRTC_LIBRARY "${WEBRTC_LIB}")
	set(WEBRTC_LIBRARIES "${WEBRTC_LIBRARY}" "${WEBRTC_PLATFORM_DEPS}")
	set(WEBRTC_LINK_DIRECTORIES "${WEBRTC_LINK_DIRECTORIES}" "${WEBRTC_PLATFORM_LINK_DIRECTORIES}")
	set(WEBRTC_DEFINITIONS_RELEASE ${WEBRTC_DEFINITIONS_RELEASE} ${WEBRTC_PLATFORM_DEFINITIONS_RELEASE})
	set(WEBRTC_DEFINITIONS_DEBUG ${WEBRTC_DEFINITIONS_DEBUG} ${WEBRTC_PLATFORM_DEFINITIONS_DEBUG})
	set(WEBRTC_COMPILE_OPTIONS_RELEASE ${WEBRTC_COMPILE_OPTIONS_RELEASE} "${WEBRTC_PLATFORM_CXX_FLAGS};${WEBRTC_PLATFORM_C_FLAGS};${WEBRTC_PLATFORM_C_FLAGS_RELEASE}")
	set(WEBRTC_COMPILE_OPTIONS_DEBUG ${WEBRTC_COMPILE_OPTIONS_DEBUG} "${WEBRTC_PLATFORM_CXX_FLAGS};${WEBRTC_PLATFORM_C_FLAGS};${WEBRTC_PLATFORM_C_FLAGS_DEBUG}")
	set(WEBRTC_LINK_OPTIONS ${WEBRTC_LINK_OPTIONS} ${WEBRTC_PLATFORM_LINKER_FLAGS})

	mark_as_advanced(WEBRTC_FOUND WEBRTC_LIBRARY WEBRTC_LIBRARIES WEBRTC_INCLUDE_DIR WEBRTC_INCLUDE_DIRS)

	if(WEBRTC_VERSION)
		message(STATUS "WEBRTC_VERSION: ${WEBRTC_VERSION}")
	endif()
	message(STATUS "WEBRTC_ROOT_DIR: ${WEBRTC_ROOT_DIR}")
	message(STATUS "WEBRTC_INCLUDE_DIR: ${WEBRTC_INCLUDE_DIR}")
	message(STATUS "WEBRTC_INCLUDE_DIRS:")
	foreach(_dir "${WEBRTC_INCLUDE_DIRS}")
		message(STATUS "-- ${_dir}")
	endforeach()
	message(STATUS "WEBRTC_LIBRARY: ${WEBRTC_LIBRARY}")
	message(STATUS "WEBRTC_LIBRARIES:")
	foreach(_lib "${WEBRTC_LIBRARIES}")
		message(STATUS "-- ${_lib}")
	endforeach()
endif()

if(WEBRTC_FOUND AND NOT TARGET WebRTC::WebRTC)
	add_library(WebRTC::WebRTC STATIC IMPORTED)
	set_target_properties(WebRTC::WebRTC PROPERTIES
		IMPORTED_LOCATION "${WEBRTC_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${WEBRTC_INCLUDE_DIRS}"
		INTERFACE_LINK_LIBRARIES "${WEBRTC_LIBRARIES}"
		# INTERFACE_LINK_DIRECTORIES "${WEBRTC_LINK_DIRECTORIES}"
		INTERFACE_COMPILE_DEFINITIONS "$<IF:$<CONFIG:Debug>,${WEBRTC_DEFINITIONS_DEBUG},${WEBRTC_DEFINITIONS_RELEASE}>"
		# INTERFACE_COMPILE_OPTIONS "$<IF:$<CONFIG:Debug>,${WEBRTC_COMPILE_OPTIONS_DEBUG},${WEBRTC_COMPILE_OPTIONS_RELEASE}>"
		INTERFACE_LINK_OPTIONS "${WEBRTC_LINK_OPTIONS}"
	)
endif()
