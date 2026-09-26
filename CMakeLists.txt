cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME
        xmrblocks)


project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

# Crow must use the Boost.Asio backend (CROW_USE_BOOST), otherwise it
# includes standalone <asio.hpp> which conflicts with Monero's
# <boost/asio/...> headers (duplicate asio:: vs boost::asio:: symbols).
add_definitions(-DCROW_USE_BOOST)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -O3")
endif()


if (NOT MONERO_DIR)
    set(MONERO_DIR ~/monero)
endif()

message(STATUS MONERO_DIR ": ${MONERO_DIR}")

if (NOT MONERO_SOURCE_DIR)
    set(MONERO_SOURCE_DIR ${MONERO_DIR}
          CACHE PATH "Path to the root directory for Monero")
endif()

if (NOT MONERO_BUILD_DIR)
    # set location of monero build tree
    set(MONERO_BUILD_DIR ${MONERO_SOURCE_DIR}/build/release/
          CACHE PATH "Path to the build directory for Monero")

   if (NOT EXISTS ${MONERO_BUILD_DIR})   
        # try different location   
        message(STATUS "Trying different folder for monero libraries")
        set(MONERO_BUILD_DIR ${MONERO_SOURCE_DIR}/build/Linux/master/release/
            CACHE PATH "Path to the build directory for Monero" FORCE)
    endif()

endif()

if (NOT EXISTS ${MONERO_BUILD_DIR})   
  message(FATAL_ERROR "Monero libraries not found in: ${MONERO_BUILD_DIR}")
endif()

set(MY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake"
        CACHE PATH "The path to the cmake directory of the current project")

list(APPEND CMAKE_MODULE_PATH "${MY_CMAKE_DIR}")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${MONERO_BUILD_DIR}"
        CACHE PATH "Add Monero directory for library searching")
include(MyUtils)

find_package(Monero)

# CMake >= 3.30 removed the built-in FindBoost module (policy CMP0167):
# use Boost's own package config files instead.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# find boost
# Note: `system` is intentionally omitted: since Boost 1.69 it is
# header-only and Boost >= 1.85 (e.g. Ubuntu resolute's 1.90) no longer
# ships boost_systemConfig.cmake, so requesting it fails the configure
# step under CMake >= 3.30 (policy CMP0167, Boost's own config files).
find_package(Boost COMPONENTS
        filesystem
        thread
        date_time
        chrono
        regex
        serialization
        program_options
        REQUIRED)

#info https://github.com/arsenm/sanitizers-cmake
find_package(Sanitizers)

if(APPLE)
    include_directories(/usr/local/opt/openssl@1.1/include)
    link_directories(/usr/local/opt/openssl@1.1/lib)
    link_directories(/usr/local/lib)
endif()

MESSAGE(STATUS "Looking for libunbound") # FindUnbound.cmake from monero repo

FIND_PATH(UNBOUND_INCLUDE_DIR
  NAMES unbound.h
  PATH_SUFFIXES include/ include/unbound/
  PATHS "${PROJECT_SOURCE_DIR}"
  ${UNBOUND_ROOT}
  $ENV{UNBOUND_ROOT}
  /usr/local/
  /usr/
)

find_library (UNBOUND_LIBRARY unbound)
if (WIN32 OR (${UNBOUND_LIBRARY} STREQUAL "UNBOUND_LIBRARY-NOTFOUND"))
    add_library(unbound STATIC IMPORTED)
    set_property(TARGET unbound PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/external/unbound/libunbound.a)
endif()

if("${Xmr_WALLET-CRYPTO_LIBRARIES}" STREQUAL "Xmr_WALLET-CRYPTO_LIBRARY-NOTFOUND")
  set(WALLET_CRYPTO "")
else()
  set(WALLET_CRYPTO ${Xmr_WALLET-CRYPTO_LIBRARIES})
endif()

# include boost headers
include_directories(${Boost_INCLUDE_DIRS})

# include monero
include_directories(${MONERO_SOURCE_DIR}/build)

include_directories("ext/mstch/include")
include_directories("ext/mstch/include/src")
include_directories("ext/crow")

# Generate build/gen/xmrblocks_version.h and put build/gen on the include
# path BEFORE subdirectories are added, so all targets (including src/myxrm)
# can find it. (Subdirectories snapshot parent includes at
# add_subdirectory() time.)
create_git_version()

# add ext/ subfolder
add_subdirectory(ext/)

# add src/ subfolder
add_subdirectory(src/)


set(SOURCE_FILES
        main.cpp)

#ADD_CUSTOM_TARGET(driver DEPENDS src/templates/index.html)

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES})

add_sanitizers(${PROJECT_NAME})

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates ${CMAKE_CURRENT_BINARY_DIR}/templates)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/css ${CMAKE_CURRENT_BINARY_DIR}/templates/css)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/partials ${CMAKE_CURRENT_BINARY_DIR}/templates/partials)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/js ${CMAKE_CURRENT_BINARY_DIR}/templates/js)

# Monero no longer always vendors miniupnpc in its build tree, so
# FindMonero.cmake may create a `miniupnpc` imported target pointing at
# Xmr_MINIUPNPC_LIBRARY-NOTFOUND. Repair it to use the system library.
find_library(SYSTEM_MINIUPNPC_LIB miniupnpc)
if(TARGET miniupnpc)
    get_target_property(_miniupnpc_loc miniupnpc IMPORTED_LOCATION)
    if(NOT _miniupnpc_loc OR _miniupnpc_loc MATCHES "NOTFOUND$")
        if(SYSTEM_MINIUPNPC_LIB)
            message(STATUS "Repairing miniupnpc target -> ${SYSTEM_MINIUPNPC_LIB}")
            set_property(TARGET miniupnpc PROPERTY IMPORTED_LOCATION ${SYSTEM_MINIUPNPC_LIB})
        else()
            message(STATUS "miniupnpc not found anywhere")
        endif()
    endif()
    unset(_miniupnpc_loc)
elseif(SYSTEM_MINIUPNPC_LIB)
    message(STATUS "Using system miniupnpc: ${SYSTEM_MINIUPNPC_LIB}")
    add_library(miniupnpc UNKNOWN IMPORTED)
    set_property(TARGET miniupnpc PROPERTY IMPORTED_LOCATION ${SYSTEM_MINIUPNPC_LIB})
endif()

set(LIBRARIES
        myxrm
        myext
        mstch
        wallet
        cryptonote_core
        cryptonote_basic
        cryptonote_protocol
        cryptonote_format_utils_basic
        blockchain_db
        device
        ${WALLET_CRYPTO} 
        multisig
        daemonizer
        blocks
        lmdb
        ringct
        ringct_basic
        fcmp_pp
        fcmp_pp_rust
        common
        mnemonics
        polyseed_wrapper
        polyseed
        utf8proc
        easylogging
        checkpoints
        cncrypto        
        miniupnpc
        version
        epee
        hardforks
        randomx
        sodium
        ${Boost_LIBRARIES}
        pthread
        unbound
        crypto
        ssl)

if(APPLE)
    set(LIBRARIES ${LIBRARIES} "-framework IOKit -framework Foundation")
else()
    set(LIBRARIES ${LIBRARIES} atomic)
endif()

find_library(UNWIND_LIBRARY unwind)
if (${UNWIND_LIBRARY} STREQUAL "UNWIND_LIBRARY-NOTFOUND")
  message (STATUS "unwind library not found")
  set (UNWIND_LIBRARY "")
else ()
  message (STATUS "Found unwind library: ${UNWIND_LIBRARY}")
endif ()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT WIN32)
    set(LIBRARIES ${LIBRARIES} ${UNWIND_LIBRARY})
endif()

if (WIN32)
    set(LIBRARIES ${LIBRARIES}
            wsock32
            ntdll
            ws2_32
            Iphlpapi
            )
else()
    set(LIBRARIES ${LIBRARIES} dl)
endif()

find_package(HIDAPI)
set(LIBRARIES ${LIBRARIES} ${HIDAPI_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
