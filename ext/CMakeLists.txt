# first build mstch template library
add_subdirectory("mstch")


# now build myext library from other files
project(myext)

set(SOURCE_HEADERS
        minicsv.h
        format.h
        dateparser.h)

set(SOURCE_FILES
        format.cc
        dateparser.cpp)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myext
        STATIC
        ${SOURCE_FILES})


