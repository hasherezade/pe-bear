# pebear_update_core: the update check, without any GUI.
#
# Deliberately links only Qt Core and Qt Network. Nothing here may depend on
# QtWidgets -- the dialog lives in updater/gui and is compiled into the PE-bear
# target instead. tests/tst_no_widgets_dependency enforces that.

set(CMAKE_AUTOMOC ON)

set (update_core_srcs
	Version.cpp
	UpdateTypes.cpp
	ReleaseClient.cpp
	UpdateSettings.cpp
	UpdateChecker.cpp
)

set (update_core_hdrs
	Version.h
	UpdateTypes.h
	ReleaseClient.h
	UpdateSettings.h
	UpdateChecker.h
)

add_library(pebear_update_core STATIC ${update_core_srcs} ${update_core_hdrs})

target_include_directories(pebear_update_core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pebear_update_core PUBLIC
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Network
)

target_compile_definitions(pebear_update_core PUBLIC PEBEAR_WITH_UPDATER=1)

# Where releases come from, as "owner/name" on GitHub. A fork sets its own so
# that its builds look at its releases. Empty keeps upstream.
set(PEBEAR_UPDATE_REPOSITORY "" CACHE STRING "GitHub repository whose releases are checked (owner/name); empty for hasherezade/pe-bear")
if(PEBEAR_UPDATE_REPOSITORY)
	if(NOT PEBEAR_UPDATE_REPOSITORY MATCHES "^[A-Za-z0-9-]+/[A-Za-z0-9._-]+$")
		message(FATAL_ERROR "PEBEAR_UPDATE_REPOSITORY must be owner/name, got '${PEBEAR_UPDATE_REPOSITORY}'")
	endif()
	target_compile_definitions(pebear_update_core PUBLIC
		PEBEAR_UPDATE_REPOSITORY="${PEBEAR_UPDATE_REPOSITORY}")
	message(STATUS "updater: releases from ${PEBEAR_UPDATE_REPOSITORY}")
endif()

if(PEBEAR_BUILD_UPDATER_TESTS)
	add_subdirectory(tests)
endif()
