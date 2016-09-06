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

add_library(crypto STATIC IMPORTED)
set_property(TARGET crypto
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcrypto.a)

add_library(cryptonote_core STATIC IMPORTED)
set_property(TARGET cryptonote_core
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcryptonote_core.a)

add_library(blockchain_db STATIC IMPORTED)
set_property(TARGET blockchain_db
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libblockchain_db.a)

add_library(lmdb STATIC IMPORTED)
set_property(TARGET lmdb
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/liblmdb.a)

add_library(ringct STATIC IMPORTED)
set_property(TARGET ringct
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libringct.a)

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
        cryptonote_core
        blockchain_db
        crypto
        blocks
        common
        lmdb
        ringct
        ${Boost_LIBRARIES}
        pthread
        unbound
        unwind
        dl)
