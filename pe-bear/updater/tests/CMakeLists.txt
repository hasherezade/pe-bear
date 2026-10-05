set(CMAKE_AUTOMOC ON)

set(updater_tests
	tst_version
	tst_releaseclient
	tst_updatesettings
	tst_updatechecker
)

foreach(test_name ${updater_tests})
	add_executable(${test_name} ${test_name}.cpp)
	target_link_libraries(${test_name}
		pebear_update_core
		Qt${QT_VERSION_MAJOR}::Core
		Qt${QT_VERSION_MAJOR}::Network
		Qt${QT_VERSION_MAJOR}::Test
	)
	add_test(NAME ${test_name} COMMAND ${test_name})
	set_tests_properties(${test_name} PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	)
endforeach()

# pebear_update_core must never pull in QtWidgets: the GUI layer is compiled
# into the application instead. This is an architectural boundary, so it is
# checked rather than merely intended.
if(UNIX)
	add_test(NAME tst_no_widgets_dependency
		COMMAND ${CMAKE_COMMAND}
			-DFILES=$<TARGET_FILE:pebear_update_core>
			-P ${CMAKE_CURRENT_SOURCE_DIR}/check_no_widgets.cmake
	)
endif()
