cmake_minimum_required(VERSION 3.0.2)

set(PROJECT_NAME
        xmrblocks)


project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)

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




if (NOT ARCH OR ARCH STREQUAL "" OR ARCH STREQUAL "native" OR ARCH STREQUAL "default")
  set(ARCH_ID "${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(ARCH_ID "${ARCH}")
endif()

if (ARCH_ID STREQUAL "aarch64")
  set(ARM 1)
  set(ARM8 1)
endif()

if(WIN32 OR ARM)
  set(OPT_FLAGS_RELEASE "-O2")
endif()

if(ARM)
  message(STATUS "Setting FPU Flags for ARM Processors")
  include(TestCXXAcceptsFlag)

  #NB NEON hardware does not fully implement the IEEE 754 standard for floating-point arithmetic
  #Need custom assembly code to take full advantage of NEON SIMD

  #Cortex-A5/9  -mfpu=neon-fp16
  #Cortex-A7/15 -mfpu=neon-vfpv4
  #Cortex-A8    -mfpu=neon
  #ARMv8  	  -FP and SIMD on by default for all ARM8v-a series, NO -mfpu setting needed

  #For custom -mtune, processor IDs for ARMv8-A series:
  #0xd04 - Cortex-A35
  #0xd07 - Cortex-A57
  #0xd08 - Cortex-A72
  #0xd03 - Cortex-A73

  if(NOT ARM8)
    CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp3-d16 CXX_ACCEPTS_VFP3_D16)
    CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp4 CXX_ACCEPTS_VFP4)
    CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=hard CXX_ACCEPTS_MFLOAT_HARD)
    CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=softfp CXX_ACCEPTS_MFLOAT_SOFTFP)
  endif()

  if(ARM8)
    CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-835769 CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
    CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-843419 CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
  endif()

  if(ARM6)
    message(STATUS "Selecting VFP for ARMv6")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp")
  endif(ARM6)

  if(ARM7)
    if(CXX_ACCEPTS_VFP3_D16 AND NOT CXX_ACCEPTS_VFP4)
      message(STATUS "Selecting VFP3 for ARMv7")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp3-d16")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp3-d16")
    endif()

    if(CXX_ACCEPTS_VFP4)
      message(STATUS "Selecting VFP4 for ARMv7")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp4")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp4")
    endif()

    if(CXX_ACCEPTS_MFLOAT_HARD)
      message(STATUS "Setting Hardware ABI for Floating Point")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard")
    endif()

    if(CXX_ACCEPTS_MFLOAT_SOFTFP AND NOT CXX_ACCEPTS_MFLOAT_HARD)
      message(STATUS "Setting Software ABI for Floating Point")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=softfp")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")
    endif()
  endif(ARM7)

  if(ARM8)
    if(CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
      message(STATUS "Enabling Cortex-A53 workaround 835769")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-835769")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-835769")
    endif()

    if(CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
      message(STATUS "Enabling Cortex-A53 workaround 843419")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-843419")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-843419")
    endif()
  endif(ARM8)

endif(ARM)


include(MyUtils)

find_package(Monero)

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

create_git_version()

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates ${CMAKE_CURRENT_BINARY_DIR}/templates)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/css ${CMAKE_CURRENT_BINARY_DIR}/templates/css)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/partials ${CMAKE_CURRENT_BINARY_DIR}/templates/partials)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/src/templates/js ${CMAKE_CURRENT_BINARY_DIR}/templates/js)

set(LIBRARIES
        myxrm
        myext
        mstch
        wallet
        blockchain_db
        device
        ${WALLET_CRYPTO}
        cryptonote_core
        cryptonote_basic
        cryptonote_format_utils_basic
        cryptonote_protocol
        multisig
        daemonizer
        blocks
        lmdb
        ringct
        ringct_basic
        common
        mnemonics
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
        curl
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
