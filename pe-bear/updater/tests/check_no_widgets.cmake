# Fails if anything that must stay out of the GUI references QtWidgets.
#
# Run as a CTest case; FILES is a ";"-separated list of binaries to inspect.
#
# Two things are checked, for two different reasons. The core library must not
# depend on QtWidgets because the widget code belongs in updater/gui, which is
# compiled into the application. The pe-bear-updater executable must not either,
# and that is the stronger requirement: it runs with no display, and it must
# never be able to put a dialog in front of the user -- least of all one asking
# for administrator rights. Checking only the library would leave that to habit,
# since the helper could acquire a progress dialog of its own without the
# library changing at all.

if(NOT FILES)
	message(FATAL_ERROR "no files given to inspect")
endif()

find_program(NM_EXECUTABLE NAMES nm llvm-nm)
if(NOT NM_EXECUTABLE)
	message(STATUS "nm not available, skipping the QtWidgets boundary check")
	return()
endif()

# Widget class names as they appear in mangled C++ symbols.
set(forbidden "QWidget" "QApplication" "QDialog" "QMessageBox" "QLayout" "QAbstractButton")
set(checked 0)

foreach(file ${FILES})
	if(NOT EXISTS "${file}")
		message(FATAL_ERROR "binary not found: ${file}")
	endif()

	execute_process(
		COMMAND ${NM_EXECUTABLE} --undefined-only "${file}"
		OUTPUT_VARIABLE symbols
		ERROR_VARIABLE nm_errors
		RESULT_VARIABLE nm_result
	)
	if(NOT nm_result EQUAL 0)
		message(STATUS "nm failed on ${file} (${nm_errors}), skipping it")
		continue()
	endif()

	set(found "")
	foreach(name ${forbidden})
		if(symbols MATCHES "${name}")
			list(APPEND found ${name})
		endif()
	endforeach()

	if(found)
		string(REPLACE ";" ", " found_str "${found}")
		get_filename_component(name "${file}" NAME)
		message(FATAL_ERROR
			"${name} depends on QtWidgets (${found_str}). "
			"Widget code belongs in updater/gui, which is built into the "
			"application; neither the update core nor the helper may reach it.")
	endif()

	math(EXPR checked "${checked} + 1")
	get_filename_component(name "${file}" NAME)
	message(STATUS "${name} is free of QtWidgets dependencies")
endforeach()

if(checked EQUAL 0)
	message(STATUS "nothing could be inspected; the boundary check did not run")
endif()
