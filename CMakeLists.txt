cmake_minimum_required(VERSION 2.8)

#list(INSERT
 #       CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(PROJECT_NAME
        xmrblocks)


project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++14")


# find boost
find_package(Boost COMPONENTS
        system
        filesystem
        thread
        date_time
        chrono
        regex
        serialization
        program_options
        date_time
        REQUIRED)


# set location of monero static libraries
set(MONERO_LIBS_DIR
        /opt/monero-dev/libs)

# set location of moneroheaders
set(MONERO_HEADERS_DIR
        /opt/monero-dev/headers)

# include monero headers
include_directories(
        ${MONERO_HEADERS_DIR}/src
        ${MONERO_HEADERS_DIR}/external
        ${MONERO_HEADERS_DIR}/contrib/epee/include
        ${MONERO_HEADERS_DIR}/external/db_drivers/liblmdb)

# get individual monero static libraries
# that are needed in this project

add_library(common STATIC IMPORTED)
set_property(TARGET common PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcommon.a)

add_library(blocks STATIC IMPORTED)
set_property(TARGET blocks PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libblocks.a)

add_library(cryptoxmr STATIC IMPORTED)
set_property(TARGET cryptoxmr
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcrypto.a)

add_library(cryptonote_core STATIC IMPORTED)
set_property(TARGET cryptonote_core
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcryptonote_core.a)

add_library(cryptonote_protocol STATIC IMPORTED)
set_property(TARGET cryptonote_protocol
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcryptonote_protocol.a)

add_library(mnemonics STATIC IMPORTED)
set_property(TARGET mnemonics
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libmnemonics.a)

add_library(blockchain_db STATIC IMPORTED)
set_property(TARGET blockchain_db
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libblockchain_db.a)

add_library(lmdb STATIC IMPORTED)
set_property(TARGET lmdb
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/liblmdb.a)

add_library(ringct STATIC IMPORTED)
set_property(TARGET ringct
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libringct.a)

add_library(wallet STATIC IMPORTED)
set_property(TARGET wallet
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libwallet.a)

# include boost headers
include_directories(${Boost_INCLUDE_DIRS})
include_directories("ext/mstch/include")

# add ext/ subfolder
add_subdirectory(ext/)

# add src/ subfolder
add_subdirectory(src/)


set(SOURCE_FILES
        main.cpp)

#ADD_CUSTOM_TARGET(driver DEPENDS src/templates/index.html)

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES})

# Get the current working branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# http://xit0.org/2013/04/cmake-use-git-branch-and-commit-details-in-project/
# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the date and time of last commit
execute_process(
        COMMAND git log -1 --format=%cd --date=short
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATETIME
        OUTPUT_STRIP_TRAILING_WHITESPACE
)


configure_file(
        ${CMAKE_SOURCE_DIR}/src/version.h.in
        ${CMAKE_BINARY_DIR}/gen/version.h
)

include_directories(${CMAKE_BINARY_DIR}/gen)

macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    @ONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates ${CMAKE_CURRENT_BINARY_DIR}/templates)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/css ${CMAKE_CURRENT_BINARY_DIR}/templates/css)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/partials ${CMAKE_CURRENT_BINARY_DIR}/templates/partials)


target_link_libraries(${PROJECT_NAME}
        myxrm
        myext
        mstch
        wallet
        cryptonote_core
        cryptonote_protocol
        blockchain_db
        cryptoxmr
        blocks
        lmdb
        ringct
        common
        mnemonics
        ${Boost_LIBRARIES}
        pthread
        unbound
        unwind
        curl
        dl
        crypto
        ssl)
