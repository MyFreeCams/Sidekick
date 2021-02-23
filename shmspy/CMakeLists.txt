

set(MyTarget shmspy)

project(${MyTarget})

#if (APPLE)
#	set (QTDIR "/usr/local/opt/qt")
#else()
#	set (QTDIR "c:/Qt/5.14.2/msvc2017_64")
#endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#set (CMAKE_PREFIX_PATH "${QTDIR}")
#set (CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}" "${QTDIR}/lib/Cmake/Qt5")
#set (Qt5_DIR "${QTDIR}/lib/Cmake/Qt5")
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#find_package(Qt5 COMPONENTS Widgets QML REQUIRED)

set(MyTarget_CORE_FILES
	main.cpp
	mfcspymain.cpp
	mfcspymain.h
	mfcspymain.ui
		QTEventHandler.h
		QTEventHandler.cpp
)

set(MyTarget_MAC_FILES

)

set(MyTarget_WIN_FILES

)

set(MyTarget_LNX_FILES

)

#qt5_wrap_cpp(QTEventHandler.cpp)
#set_property(SOURCE QTEventHandler.cpp PROPERTY SKIP_AUTOMOC ON)

if (APPLE)
	add_executable(${MyTarget}
		${MyTarget_CORE_FILES}
		${MyTarget_MAC_FILES}
	)
elseif(WIN32)
	add_executable(${MyTarget}
		${MyTarget_CORE_FILES}
		${MyTarget_WIN_FILES}
	)

	target_link_libraries(${MyTarget} PUBLIC
		ws2_32
			boost::filesystem
	)
else()
	add_executable(${MyTarget}
		${MyTarget_CORE_FILES}
		${MyTarget_LNX_FILES}
	)
endif()

target_include_directories(${MyTarget} PUBLIC
		../libipc
		../libfcs
		${Boost_INCLUDE_DIRS}
		#${Qt5QML_INCLUDE_DIRS}
		)

target_link_directories(${MyTarget} PUBLIC
		${Boost_LIBRARY_DIRS}
		#${Qt5QML_INCLUDE_DIRS}
		)
message("BOOST Libraries ${Boost_LIBRARIES}")
# BOOST Libraries /usr/local/opt/boost/lib/libboost_date_time-mt.a;/usr/local/opt/boost/lib/libboost_thread-mt.a
target_link_libraries(${MyTarget} PUBLIC
		libipc
		MFClibfcs
		${Boost_LIBRARIES}
		Qt5::Widgets
		Qt5::Core
		#Qt5::Qml
		)

target_compile_definitions(${MyTarget} PUBLIC
		#${Qt5Qml_DEFINITIONS}
		MFCSPY_MAINTENANCE_BID=7
		)

if (APPLE)
	add_custom_command(TARGET ${MyTarget} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_CURRENT_BINARY_DIR}/../libipc/libipc.dylib"			"${CMAKE_CURRENT_BINARY_DIR}/libipc.dylib"
			)
endif ()

if (DO_NOT_USE_CRASHES)
add_custom_command(TARGET ${MyTarget} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_date_time-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_date_time-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_thread-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_thread-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_filesystem-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${Boost_LIBRARY_DIRS}/boost_filesystem-vc141-mt-gd-x64-1_69.dll" "${CMAKE_CURRENT_BINARY_DIR}"

)
add_custom_command(TARGET ${MyTarget} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Widgets.dll"			"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Widgets.dll"	"${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Widgetsd.dll"			"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Widgetsd.dll"	"${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Core.dll"			"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Cored.dll"	"${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Core.dll"			"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Cored.dll"	"${CMAKE_CURRENT_BINARY_DIR}"
)
add_custom_command(TARGET ${MyTarget} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Guid.dll"			"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
	COMMAND ${CMAKE_COMMAND} -E copy  "${QTDIR}/bin/Qt5Guid.dll"	"${CMAKE_CURRENT_BINARY_DIR}"

)
message(${BOOST_ROOT})
message(${Boost_LIBRARY_DIRS})
message(${CMAKE_BINARY_DIR})
message(${CMAKE_CURRENT_BINARY_DIR})
message(${QTDIR})
endif()