#S:/PE-bear/pe-bear/gui

cmake_minimum_required(VERSION 2.8)

find_package (Qt4 REQUIRED)

set (MY_DIR gui)

set ( my_hdrs
	ContentPreview.h
	DetailsTab.h
	DosHdrTableModel.h
	GeneralPanel.h
	SectionAddWindow.h
)

set ( my_cpp
	ContentPreview.cpp
	DetailsTab.cpp
	DosHdrTableModel.cpp
	GeneralPanel.cpp
	SectionAddWindow.cpp
)

#qt4_wrap_cpp( my_hdrs_moc ${my_hdrs} )

SET(resfile_list "")
foreach(file ${my_hdrs})
	SET(resfile_list ${resfile_list} ${MY_DIR}/${file})
	message(STATUS "file='${resfile_list}'")
endforeach()

set (gui_h
	${resfile_list}
	PARENT_SCOPE
)

SET(resfile_list "")
foreach(file ${my_cpp})
	SET(resfile_list ${resfile_list} ${MY_DIR}/${file})
	message(STATUS "file='${resfile_list}'")
endforeach()

set (gui_cpp
	${resfile_list}
	PARENT_SCOPE
)