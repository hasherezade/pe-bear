cmake_minimum_required(VERSION 3.0)
project (PE-bear)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(COMMIT_HASH "" CACHE STRING "Commit Hash")
set(SCHEME_NAME "net.hasherezade.pe-bear")

message(STATUS "sigfind_dir='${SIGFIND_DIR}'")
message(STATUS "sigfind_lib='${SIGFIND_LIB}'")

message(STATUS "parser_dir='${PARSER_DIR}'")
message(STATUS "parser_lib='${PARSER_LIB}'")

message(STATUS "disasm_dir='${DISASM_DIR}'")
message(STATUS "disasm_lib='${DISASM_DIR}'")

include(GNUInstallDirs)

if(USE_QT4)
	find_package (Qt4 REQUIRED)
	include_directories ( ${QT_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} )
	INCLUDE(${QT_USE_FILE})
	ADD_DEFINITIONS(${QT_DEFINITIONS})
else()
	find_package(Qt5Core REQUIRED)
	find_package(Qt5Widgets REQUIRED)
	get_target_property(QtCore_location Qt5::Core LOCATION)
endif()

include_directories( ${PARSER_INC} )
include_directories( ${SIGFIND_INC} )
include_directories( ${DISASM_DIR} ${CAPSTONE_INC})

set (pebear_threads_srcs
	base/threads/CalcThread.cpp
	base/threads/SignFinderThread.cpp
	base/threads/StringExtThread.cpp
)

set (pebear_threads_hdrs
	base/threads/CollectorThread.h
	base/threads/CalcThread.h
	base/threads/SignFinderThread.h
	base/threads/StringExtThread.h
)

set (pebear_base_srcs
	base/BearVers.cpp
	base/MainSettings.cpp
	base/CommentHandler.cpp
	base/Modification.cpp
	base/PeHandler.cpp
	base/PeHandlerFactory.cpp
	base/PeHandlersManager.cpp
	base/RegKeyManager.cpp
)

set (pebear_base_hdrs
	base/BearVers.h
	base/MainSettings.h
	base/CommentHandler.h
	base/Modification.h
	base/Releasable.h
	base/PeHandler.h
	base/PeHandlerFactory.h
	base/PeHandlersManager.h
	base/StringsCollection.h
	base/RegKeyManager.h
	base/ImportsAutoadderSettings.h
)

set (pebear_gui_base_srcs
	gui_base/PeGuiItem.cpp
	gui_base/ExtTableView.cpp
	gui_base/PEViewsManager.cpp
	gui_base/ClipboardUtil.cpp
	gui_base/TreeCpView.cpp
	gui_base/PeTreeView.cpp
	gui_base/PeTableModel.cpp
	gui_base/FollowablePeTreeView.cpp
	gui_base/HexInputDialog.cpp
	gui_base/AddressInputDialog.cpp
	gui_base/OffsetDependentAction.cpp
	gui_base/WrapperTableModel.cpp
	gui_base/WrapperInterface.cpp
	gui_base/HexSpinBox.cpp	
)

set (pebear_gui_base_hdrs
	gui_base/PeGuiItem.h
	gui_base/ExtTableView.h
	gui_base/PEViewsManager.h
	gui_base/ClipboardUtil.h
	gui_base/TreeCpView.h
	gui_base/PeTreeView.h
	gui_base/PeTableModel.h
	gui_base/FollowablePeTreeView.h
	gui_base/HexInputDialog.h
	gui_base/AddressInputDialog.h
	gui_base/OffsetDependentAction.h
	gui_base/WrapperTableModel.h
	gui_base/WrapperInterface.h
	gui_base/HexSpinBox.h
)

set (pebear_gui_pe_mod_srcs
	gui/pe_models/BoundImpTreeModel.cpp
	gui/pe_models/DebugTreeModel.cpp
	gui/pe_models/DelayImpTreeModel.cpp
	gui/pe_models/ExceptionTreeModel.cpp
	gui/pe_models/ExportsTreeModel.cpp
	gui/pe_models/FileHdrTreeModel.cpp
	gui/pe_models/RichHdrTreeModel.cpp
	gui/pe_models/ImportsTreeModel.cpp
	gui/pe_models/LdConfigTreeModel.cpp
	gui/pe_models/OptionalHdrTreeModel.cpp
	gui/pe_models/RelocsTreeModel.cpp
	gui/pe_models/ResourcesTreeModel.cpp
	gui/pe_models/SecHdrsTreeModel.cpp
	gui/pe_models/SecurityTreeModel.cpp
	gui/pe_models/TLSTreeModel.cpp
	gui/pe_models/ClrHdrTreeModel.cpp
)

set (pebear_gui_pe_mod_hdrs
	gui/pe_models/BoundImpTreeModel.h
	gui/pe_models/DebugTreeModel.h
	gui/pe_models/DelayImpTreeModel.h
	gui/pe_models/ExceptionTreeModel.h
	gui/pe_models/ExportsTreeModel.h
	gui/pe_models/FileHdrTreeModel.h
	gui/pe_models/RichHdrTreeModel.h
	gui/pe_models/ImportsTreeModel.h
	gui/pe_models/LdConfigTreeModel.h
	gui/pe_models/OptionalHdrTreeModel.h
	gui/pe_models/RelocsTreeModel.h
	gui/pe_models/ResourcesTreeModel.h
	gui/pe_models/SecHdrsTreeModel.h
	gui/pe_models/SecurityTreeModel.h
	gui/pe_models/TLSTreeModel.h
	gui/pe_models/ClrHdrTreeModel.h
	gui/DarkStyle.h
)

set (pebear_gui_srcs
	gui/CommentView.cpp
	gui/DetailsTab.cpp
	gui/ContentPreview.cpp
	gui/GeneralPanel.cpp
	gui/DosHdrTableModel.cpp
	gui/PackersTableModel.cpp
	gui/HexDiffModel.cpp
	gui/WrapperSplitter.cpp
	gui/DataDirWrapperSplitter.cpp
	gui/ResourceDirSplitter.cpp
	gui/WrapperTreeView.cpp
	gui/TreeModel.cpp
	gui/PeTreeModel.cpp
	gui/PeWrapperModel.cpp
	DateDisplay.cpp
	HexView.cpp
	HexCompareView.cpp
	HexDumpModel.cpp
	OffsetHeader.cpp
	DisasmView.cpp
	PEDockedWidget.cpp
	PEFileTreeModel.cpp
	SectionsDiagram.cpp
	ExeDependentAction.cpp
	ViewSettings.cpp
)

set (pebear_gui_hdrs
	gui/CommentView.h
	gui/DetailsTab.h
	gui/ContentPreview.h
	gui/GeneralPanel.h
	gui/DosHdrTableModel.h
	gui/PackersTableModel.h
	gui/HexDiffModel.h
	gui/WrapperSplitter.h
	gui/DataDirWrapperSplitter.h
	gui/ResourceDirSplitter.h
	gui/WrapperTreeView.h
	gui/TreeModel.h
	gui/PeTreeModel.h
	gui/PeWrapperModel.h
	DateDisplay.h
	HexView.h
	HexCompareView.h
	HexDumpModel.h
	OffsetHeader.h
	DisasmView.h
	PEDockedWidget.h
	PEFileTreeModel.h
	SectionsDiagram.h
	ExeDependentAction.h
	ViewSettings.h
	TempBuffer.h
	gui/pe_models.h
)

set (pebear_followable_srcs
	gui/followable_table/OffsetedView.cpp
)

set (pebear_followable_hdrs
	gui/followable_table/MouseTrackingTableView.h
	gui/followable_table/FollowableOffsetedView.h
	gui/followable_table/OffsetedView.h
)

set (pebear_windows_srcs
	gui/windows/PatternSearchWindow.cpp
	gui/windows/StringsBrowseWindow.cpp
	gui/windows/OffsetsBrowseWindow.cpp
	gui/windows/SignaturesBrowseWindow.cpp
	gui/windows/SectionAddWindow.cpp
	gui/windows/ImportsAddWindow.cpp
	gui/windows/UserConfigWindow.cpp
	gui/windows/DiffWindow.cpp
	gui/windows/MainWindow.cpp
)

set (pebear_windows_hdrs
	gui/windows/PatternSearchWindow.h
	gui/windows/StringsBrowseWindow.h
	gui/windows/OffsetsBrowseWindow.h
	gui/windows/SignaturesBrowseWindow.h
	gui/windows/SectionAddWindow.h
	gui/windows/ImportsAddWindow.h
	gui/windows/UserConfigWindow.h
	gui/windows/DiffWindow.h
	gui/windows/MainWindow.h
)

set (pebear_srcs
	${pebear_threads_srcs}
	${pebear_base_srcs}
	${pebear_gui_base_srcs}
	${pebear_gui_srcs}
	${pebear_gui_pe_mod_srcs}
	${pebear_windows_srcs}
	${pebear_followable_srcs}
	main.cpp
)

set (pebear_hdrs
	REbear.h
	${pebear_threads_hdrs}
	${pebear_base_hdrs}
	${pebear_gui_base_hdrs}
	${pebear_gui_hdrs}
	${pebear_gui_pe_mod_hdrs}
	${pebear_windows_hdrs}
	${pebear_followable_hdrs}
)

set (pebear_rcc application.qrc )

if(USE_QT4)
	qt4_add_resources(pebear_rcc_src ${pebear_rcc})
	qt4_wrap_cpp(pebear_hdrs_moc ${pebear_hdrs})
else()
	qt5_add_resources(pebear_rcc_src ${pebear_rcc})
	qt5_wrap_cpp(pebear_hdrs_moc ${pebear_hdrs})
endif()

SOURCE_GROUP("Source Files\\base" FILES ${pebear_base_srcs} )
SOURCE_GROUP("Source Files\\gui" FILES ${pebear_gui_srcs} )
SOURCE_GROUP("Source Files\\gui\\pe_models" FILES ${pebear_gui_pe_mod_srcs} )
SOURCE_GROUP("Source Files\\gui\\windows" FILES ${pebear_windows_srcs} )
SOURCE_GROUP("Source Files\\gui_base" FILES ${pebear_gui_base_srcs} )
SOURCE_GROUP("Source Files\\Auto Generated" FILES ${pebear_hdrs_moc} ${pebear_rcc_src} )

SOURCE_GROUP("Header Files\\base" FILES ${pebear_base_hdrs} )
SOURCE_GROUP("Header Files\\gui" FILES ${pebear_gui_hdrs} )
SOURCE_GROUP("Header Files\\gui\\pe_models" FILES ${pebear_gui_pe_mod_hdrs} )
SOURCE_GROUP("Header Files\\gui\\windows" FILES ${pebear_windows_hdrs} )
SOURCE_GROUP("Header Files\\gui_base" FILES ${pebear_gui_base_hdrs} )

IF (WIN32)
	IF(NOT SHOW_CONSOLE)
		#console hidden
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	ENDIF()
ENDIF()

add_definitions(-DCOMMIT_HASH=${COMMIT_HASH})

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(MACOSX TRUE)
ENDIF()

IF(MACOSX)
	add_executable (${PROJECT_NAME} MACOSX_BUNDLE resources.rc ${pebear_hdrs} ${pebear_srcs} ${pebear_hdrs_moc} ${pebear_rcc_src} )

	set_target_properties(${PROJECT_NAME} PROPERTIES
	    BUNDLE True
	    #MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${PROJECT_NAME}
	    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
	    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
	    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
	    #MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/customtemplate.plist.in
	)
ELSE()
	add_executable (${PROJECT_NAME} resources.rc ${pebear_hdrs} ${pebear_srcs} ${pebear_hdrs_moc} ${pebear_rcc_src} )
ENDIF()

target_link_libraries( ${PROJECT_NAME} ${SIGFIND_LIB} ${PARSER_LIB} )
target_link_libraries (${PROJECT_NAME}  ${DISASM_LIB} ${CAPSTONE_LIB} )

if(USE_QT4)
	target_link_libraries (${PROJECT_NAME} ${QT_QTGUI_LIBRARIES} ${QT_QTCORE_LIBRARIES})
else()	
	target_link_libraries (${PROJECT_NAME} Qt5::Core Qt5::Widgets )
endif()

install(TARGETS ${PROJECT_NAME}
	BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT ${PROJECT_NAME}
)

IF (NOT WIN32)
	install(FILES "${CMAKE_SOURCE_DIR}/logo/main_ico.png"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
		RENAME "${SCHEME_NAME}.png"
	)
	configure_file("${CMAKE_SOURCE_DIR}/xdg/launcher.desktop.in"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
		@ONLY
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		RENAME "${SCHEME_NAME}.desktop"
	)

	configure_file("${CMAKE_SOURCE_DIR}/xdg/appdata.metainfo.xml.in"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.metainfo.xml"
		@ONLY
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.metainfo.xml"
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
		RENAME "${SCHEME_NAME}.metainfo.xml"
	)
endif()
