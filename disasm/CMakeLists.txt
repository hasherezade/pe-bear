cmake_minimum_required(VERSION 3.0)
project (libdisasm)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")

if(USE_QT4)
	find_package (Qt4 REQUIRED)
	include_directories( ${QT_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR} )
	INCLUDE( ${QT_USE_FILE} )
	ADD_DEFINITIONS( ${QT_DEFINITIONS} )
else()
	find_package(Qt5Core REQUIRED)
	get_target_property(QtCore_location Qt5::Core LOCATION)
endif()

include_directories ( ${UDIS86_DIR} ${CAPSTONE_INC} )
include_directories( ${PARSER_INC} )

if (USE_UDIS86)
	set (disasm_wrappers_srcs
		udis/UDisasm.cpp
	)
	set (disasm_wrappers_hdrs
		udis/UDisasm.h
	)
	message(STATUS "udis_dir='${UDIS86_DIR}'")
	message(STATUS "udis_lib='${UDIS86_LIB}'")
else()
	set (disasm_wrappers_srcs
		cdis/CDisasm.cpp
	)
	set (disasm_wrappers_hdrs
		cdis/CDisasm.h
	)
	message(STATUS "capstone_includes='${CAPSTONE_INC}'")
	message(STATUS "capstone_lib='${CAPSTONE_LIB}'")
endif ()


set (disasm_srcs
	Disasm.cpp
	PeDisasm.cpp
	${disasm_wrappers_srcs}
)

set (disasm_hdrs
	bear_disasm.h
	Disasm.h
	PeDisasm.h
	MnemType.h
	${disasm_wrappers_hdrs}
)

add_library( ${PROJECT_NAME} STATIC ${disasm_srcs} ${disasm_hdrs} )

if (USE_UDIS86)
    set (def
        "BUILD_WITH_UDIS86"
    )
    list(APPEND deflist ${def})
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "${deflist}")
endif()

if (USE_UDIS86)
    target_link_libraries( ${PROJECT_NAME} ${PARSER_LIB} ${UDIS86_LIB} )
else()
    target_link_libraries( ${PROJECT_NAME} ${PARSER_LIB} capstone-static )
endif()

if(USE_QT4)
	target_link_libraries ( ${PROJECT_NAME} ${QT_QTCORE_LIBRARIES} )
else()
	target_link_libraries( ${PROJECT_NAME} Qt5::Core)
endif()

