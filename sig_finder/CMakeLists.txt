cmake_minimum_required(VERSION 3.0)
project (sig_finder)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")

set (sig_srcs
	sig_finder/SigNode.cpp
	sig_finder/PckrSign.cpp
	sig_finder/SigTree.cpp
	sig_finder/SigFinder.cpp
)

set (sig_hdrs
	sig_finder/SigNode.h
	sig_finder/PckrSign.h
	sig_finder/SigTree.h
	sig_finder/SigFinder.h
	sig_finder.h
)
add_library(${PROJECT_NAME} STATIC ${sig_srcs} ${sig_hdrs})

