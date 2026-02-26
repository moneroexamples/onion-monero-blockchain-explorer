cmake_minimum_required(VERSION 3.0.2)

project(myxrm)

set(SOURCE_HEADERS
        MicroCore.h
		tools.h
		monero_headers.h
		CurrentBlockchainStatus.h)

set(SOURCE_FILES
		MicroCore.cpp
		tools.cpp
		CmdLineOptions.cpp
		page.h
		rpccalls.cpp rpccalls.h
		version.h.in 
        CurrentBlockchainStatus.cpp 
        MempoolStatus.cpp 
        MempoolStatus.h)

add_subdirectory(crypto)


# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myxrm
		STATIC
		${SOURCE_FILES})

target_link_libraries(myxrm mycrypto)

