cmake_minimum_required(VERSION 3.0.2)

project(mycrypto)

set(SOURCE_FILES
    rx-slow-hash.c)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(mycrypto
		STATIC
		${SOURCE_FILES})

