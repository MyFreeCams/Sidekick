#######################################
#  ObsUpdater                         #
#  -(description)                     #
#######################################
#  Target: MFCUpdater                 #
#  CMAKE_SOURCE_DIR  : ../../../..    #
#  PROJECT_SOURCE_DIR: ../../../..    #
#######################################

set(MyTarget MFCUpdater)

#
# Source files.
#
set(MyTarget_CORE_FILES
	dllmain.cpp
	FileUpdater.h
	FileUpdater.cpp
	ManifestFile.h
	ManifestFile.cpp
	MFCUpdaterAPI.h
	MFCUpdaterAPI.cpp
	MFC-OBSPluginStart.cpp
	ObsUpdater.h
)
set(MyTarget_MAC_FILES
)
set(MyTarget_WIN_FILES
	MFCUpdater.dll.manifest
	resource.h
	targetver.h
	_MFC-OBSPluginUpdater.rc
)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if(APPLE)
	add_library(${MyTarget} MODULE
		${MyTarget_CORE_FILES}
		${MyTarget_MAC_FILES}
	)
elseif(WIN32)
	add_library(${MyTarget} MODULE
		${MyTarget_CORE_FILES}
		${MyTarget_WIN_FILES}
	)
endif()

#add_dependencies(${MyTarget} obs)

set_target_properties(${MyTarget} PROPERTIES PREFIX "")

target_compile_options(${MyTarget} PUBLIC
	-D_CRT_SECURE_NO_WARNINGS
	-D_UNICODE
	-DUNICODE
)

MFCDefines(${MyTarget})

# target_include_directories(${MyTarget} PUBLIC ${Boost_INCLUDE_DIR})
# target_link_directories(${MyTarget} PUBLIC ${Boost_LIBRARY_DIRS})

target_link_libraries(${MyTarget} PUBLIC
	libobs
	MFClibfcs
	MFCLibPlugins
	obs-frontend-api
	Qt5::Widgets
	CURL::libcurl
)

if(APPLE)
	target_link_libraries(${MyTarget} PRIVATE objc)
else()
	target_link_libraries(${MyTarget} PRIVATE
		version.lib
		w32-pthreads
		Wbemuuid.lib
		Winhttp.lib
		Wininet.lib
	)
endif()

if(APPLE)
	# not installing for now (Kiran 12/25/19)
	# install_obs_plugin(${MyTarget})

	# add_custom_command(TARGET ${MyTarget}
	#   POST_BUILD
	#   COMMAND ${CMAKE_COMMAND} -E copy
	#       ${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/ObsUpdater/MFCUpdater.so
	#       ${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/obs-plugins/MFCUpdater.so
	# )
elseif(WIN32)
	# 1) turn "MFCUpdater" into "ObsUpdater"
	# 2) convert cmake path to native filesystem path for windows,
	# 3) copy output file and pdb if it exists to the sidekick bindir

	STRING(REGEX REPLACE "MFC" "Obs" PROJECT_DIRNAME ${MyTarget})

	# set(OUTFILE "${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/${PROJECT_DIRNAME}/$<CONFIG>/${MyTarget}.dll")
	set(OUTFILE $<TARGET_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH "${OUTFILE}" NATIVE_TARGET_FILE)

	set(PDBFILE $<TARGET_PDB_FILE:${MyTarget}>)
	file(TO_NATIVE_PATH "${PDBFILE}" NATIVE_TARGET_PDB)

	# ${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/obs-plugins/64bit/
	file(TO_NATIVE_PATH "${OBS_RUNDIR_PLUGIN_PATH}" NATIVE_OBS_RUNDIR_PLUGIN_PATH)

	set(UPDATER_BUILD_PATH "${CMAKE_BINARY_DIR}/MFCUpdater")
	file(TO_NATIVE_PATH "${UPDATER_BUILD_PATH}" NATIVE_UPDATER_BUILD_PATH)

	# Use OBS_APP_INST_PATH instead of MFC_OBS_PLUGIN_BIN_PATH since we are copying to obs path for MFCUpdater!
	file(TO_NATIVE_PATH "${OBS_APP_INST_PATH}/obs-plugins/64bit/${MyTarget}.dll" NATIVE_OBS_PATH)

	# create rundir plugin path & build directory
	add_custom_command(TARGET ${MyTarget}
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_OBS_RUNDIR_PLUGIN_PATH}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_UPDATER_BUILD_PATH}
	)

	# copy MFCUpdater.dll to obs-plugins directory in rundir
	message(STATUS "OUTPUT COPY: ${NATIVE_TARGET_FILE} => ${NATIVE_OBS_RUNDIR_PLUGIN_PATH}")
	message(STATUS "PDB    COPY: ${NATIVE_TARGET_PDB} => ${NATIVE_OBS_RUNDIR_PLUGIN_PATH}")
	add_custom_command(TARGET ${MyTarget}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MyTarget}> ${OBS_RUNDIR_PLUGIN_PATH}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${MyTarget}> ${OBS_RUNDIR_PLUGIN_PATH}
	)

	# copy MFCUpdater.dll to build directory (for inno setup packaging)
	message(STATUS "OUTPUT COPY: ${NATIVE_TARGET_FILE} => ${NATIVE_UPDATER_BUILD_PATH}")
	message(STATUS "PDB    COPY: ${NATIVE_TARGET_PDB} => ${NATIVE_UPDATER_BUILD_PATH}")
	add_custom_command(TARGET ${MyTarget}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MyTarget}> ${UPDATER_BUILD_PATH}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${MyTarget}> ${UPDATER_BUILD_PATH}
	)

	# this custom command executes the AdminCopy1.link shortcut, a shortcut that runs a task manager task
	# that executes the copy with admin privileges so that it can be written to the program files dir.

	# message(STATUS "OUTPUT_COPY: ${NATIVE_TARGET_FILE} => ${NATIVE_OBS_PATH}")
	# add_custom_command(TARGET ${MyTarget}
	# 	POST_BUILD
	# 	COMMAND START C:\\Users\\OBS\\AdminCopy1.lnk)
	# 	${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_FILE} ${NATIVE_OBS_PATH}
	# )

	# copy pdb file install directory
	# set(PDBFILE "${CMAKE_BINARY_DIR}/plugins/MyFreeCams/Sidekick/${PROJECT_DIRNAME}/$<CONFIG>/${MyTarget}.pdb")
	# file(TO_NATIVE_PATH "${PDBFILE}" NATIVE_TARGET_PDB)
	# if(EXISTS ${NATIVE_TARGET_PDB})
	# 	message(STATUS "PDB_COPY: ${NATIVE_TARGET_PDB} => ${NATIVE_OBS_PATH}")
	# 	add_custom_command(TARGET ${MyTarget}
	# 		POST_BUILD
	# 		COMMAND ${CMAKE_COMMAND} -E copy ${NATIVE_TARGET_PDB} ${NATIVE_OBS_PATH})
	# endif()
endif()
