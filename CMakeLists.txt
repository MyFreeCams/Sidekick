#######################################
#  Sidekick Root                      #
#  -(description)                     #
#######################################
#  CMAKE_SOUECE_DIR   ../../..        #
#  PROJECT_SOURCE_DIR ../../..        #
#######################################
#  Subdirectory      Target           #
#  ---------------------------------  #
#  libcef_fcs        MFCLibCef        #
#  libfcs            MFClibfcs        #
#  libPlugins        MFCLibPlugins    #
#  mfc-browser       mfc-browser      #
#  MFCCefLogin       MFCCefLogin      #
#  ObsBroadcast      MFCBroadCast     #
#  ObsUpdater        MFCUpdater       #
#  websocket-client  websocketclient  #
#######################################

cmake_minimum_required(VERSION 3.13)

# fixes IF() with quoted args
cmake_policy(SET CMP0054 NEW)
# honor visibility properties for all target types
cmake_policy(SET CMP0063 NEW)
# ignore the ``RPATH`` settings for ``install_name`` on macOS
cmake_policy(SET CMP0068 NEW)
# find_package now searches prefixes specified by the
#   ``<PackageName>_ROOT`` CMake variable and the
#   ``<PackageName>_ROOT`` environment variable.
cmake_policy(SET CMP0074 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PROJECT_ARCH "x86_64")
	set(_bin_suffix 64)
	set(_lib_suffix 64)
else()
	set(PROJECT_ARCH "x86")
	set(_bin_suffix 32)
	set(_lib_suffix 32)
endif()

if(WIN32)
	STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
	set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "/ignore:4099 /ignore:4840 /ignore:4003")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /ignore:4099 /ignore:4840 /ignore:4003")
	add_compile_options($<IF:$<CONFIG:Debug>,/MTd,/MT>)
	add_compile_options(/wd4005 /wd4996 /wd4003 /wd4840 /wd4100)
	add_compile_definitions(_HAS_STD_BYTE=0)
elseif(APPLE)
	set(CMAKE_CXX_VISIBILITY_PRESET default)
endif()

include("helpers")
include("variables")
include(FetchContent)

APPLY_PATCHES() # apply patches to obs-studio
BUILD_INFO()    # generate build version & time
SET_VARIABLES() # variables located in cmake/variables.cmake

#------------------------------------------------------------------------
# Boost
#
if (APPLE)
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_DEBUG_LIBS OFF)
	set(Boost_USE_RELEASE_LIBS ON)
	# set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_DEBUG_RUNTIME OFF)
	set(BOOST_NO_EXCEPTIONS ON)
else()
	set(Boost_USE_DEBUG_LIBS OFF)
	set(Boost_USE_RELEASE_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_DEBUG_RUNTIME OFF)
endif()

if(APPLE)
	find_package(Boost 1.69 COMPONENTS program_options date_time thread-mt system filesystem QUIET)
	if(NOT Boost_FOUND)
		#program_options date_time chrono thread filesystem
		find_package(Boost 1.74 COMPONENTS program_options date_time thread system filesystem chrono REQUIRED)
	endif()
else()
	find_package(Boost 1.69 COMPONENTS program_options date_time system chrono thread  filesystem REQUIRED)
endif()
PRINT(Boost)

#------------------------------------------------------------------------
# CURL
#
find_package(CURL REQUIRED)
PRINT(CURL)

#------------------------------------------------------------------------
# Qt
#
if(QTDIR)
	set(Qt5_DIR "${QTDIR}/lib/cmake/Qt5" CACHE PATH "Location of Qt5Config.cmake file")
	set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${Qt5_DIR}")
	set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" "${QTDIR}")
endif()
find_package(Qt5 COMPONENTS Core Widgets Gui Svg REQUIRED)
PRINT(Qt5)

#------------------------------------------------------------------------
# Nlohmann JSON (Modern JSON for C++)
#
message(STATUS "Fetching nlohmann json")
FetchContent_Declare(nlohmannjson GIT_REPOSITORY https://github.com/nlohmann/json)
FetchContent_GetProperties(nlohmannjson)
if(NOT nlohmannjson_POPULATED)
	FetchContent_Populate(nlohmannjson)
endif()

#------------------------------------------------------------------------
# WebSocket++ (websocketpp)
#
message(STATUS "Fetching websocketpp")
FetchContent_Declare(websocketpp GIT_REPOSITORY https://github.com/zaphoyd/websocketpp)
FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
	FetchContent_Populate(websocketpp)
endif()

#------------------------------------------------------------------------
# Asio C++ Library
#
message(STATUS "Fetching ASIO")
FetchContent_Declare(asio GIT_REPOSITORY https://github.com/chriskohlhoff/asio)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
	FetchContent_Populate(asio)
endif()

#------------------------------------------------------------------------
# Common include directories
#
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR} # obs-studio root directory
	${CMAKE_SOURCE_DIR}/libobs
	#${CMAKE_SOURCE_DIR}/UI
	#${CMAKE_BINARY_DIR}/UI
	${CMAKE_SOURCE_DIR}/UI/obs-frontend-api
	${CMAKE_BINARY_DIR}/UI/obs-frontend-api
	${Boost_INCLUDE_DIR}
	${CURL_INCLUDE_DIRS}
	${nlohmannjson_SOURCE_DIR}/single_include
)

#------------------------------------------------------------------------
# Subdirectories
#
add_subdirectory(libipc)
add_subdirectory(libfcs)
add_subdirectory(libPlugins)
add_subdirectory(websocket-client)
add_subdirectory(ObsBroadcast)
#add_subdirectory(shmspy)

#------------------------------------------------------------------------
# CEF Login App and/or Browser Panel
#
if(MFC_DO_CEF_BUILD)
	if(NOT DEFINED CEF_ROOT)
		if(DEFINED ENV{CEF_ROOT})
			set(CEF_ROOT "$ENV{CEF_ROOT}")
		elseif(DEFINED CEF_ROOT_DIR)
			set(CEF_ROOT "${CEF_ROOT_DIR}")
		endif()
	endif()
	set(CEF_ROOT ${CEF_ROOT} CACHE PATH "CEF Root Directory" FORCE)
	set(CEF_ROOT_DIR ${CEF_ROOT} CACHE PATH "CEF Root Directory" FORCE)
	message(STATUS "CEF_ROOT: ${CEF_ROOT}")

	option(CEF_SANDBOX "Use CEF Sandbox" OFF)
	if(CEF_SANDBOX)
		set(USE_SANDBOX ON)
	else()
		set(USE_SANDBOX OFF)
	endif()

	if(MFC_BROWSER_AVAILABLE)
		add_subdirectory(mfc-browser)
	endif()

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

	find_package(CEF REQUIRED)

	set(CEF_BINARY_DIR $<IF:$<CONFIG:Debug>,${CEF_BINARY_DIR_DEBUG},${CEF_BINARY_DIR_RELEASE}>)

	message(STATUS "CMAKE_CXX_FLAGS (before): ${CMAKE_CXX_FLAGS}")
	message(STATUS "CEF_CXX_COMPILER_FLAGS (before): ${CEF_CXX_COMPILER_FLAGS}")
	if(APPLE)
		# enable RTTI, disable warnings as errors
		list(REMOVE_ITEM CEF_CXX_COMPILER_FLAGS -fno-rtti -fno-exceptions -funwind-tables -fno-asynchronous-unwind-tables)
	elseif(WIN32)
		# enable RTTI, disable warnings as errors
		list(REMOVE_ITEM CEF_COMPILER_FLAGS /WX /GR-)
		list(APPEND CEF_COMPILER_FLAGS /WX- /GR)
		set(CEF_RUNTIME_LIBRARY_FLAG "/MT")
		add_compile_options("/GR")
		add_compile_options("/WX-")
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			add_compile_options("/MTd")
		else()
			add_compile_options("/MT")
		endif()
	endif()
	message(STATUS "CMAKE_CXX_FLAGS (after): ${CMAKE_CXX_FLAGS}")
	message(STATUS "CEF_CXX_COMPILER_FLAGS (after): ${CEF_CXX_COMPILER_FLAGS}")

	if(NOT MFC_BROWSER_LOGIN)
		add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper) # location: ${CEF_ROOT}/libcef_dll
		add_subdirectory(libcef_fcs)
		add_subdirectory(MFCCefLogin)
	endif()
	PRINT_CEF_CONFIG()
else()
	message(STATUS "Skipping MFC CEF App build")
endif()
