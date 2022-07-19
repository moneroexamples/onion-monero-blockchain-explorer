#------------------------------------------------------------------------------
# CMake helper for the majority of the cpp-ethereum modules.
#
# This module defines
#     Monero_XXX_LIBRARIES, the libraries needed to use ethereum.
#     Monero_FOUND, If false, do not try to use ethereum.
#
# File addetped from cpp-ethereum
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2014-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

set(LIBS common;blocks;cryptonote_basic;cryptonote_core;multisig;
		cryptonote_protocol;daemonizer;mnemonics;epee;lmdb;device;wallet-crypto;
		blockchain_db;ringct;wallet;cncrypto;easylogging;version;
        checkpoints;randomx;hardforks;miniupnpc)

set(Xmr_INCLUDE_DIRS "${CPP_MONERO_DIR}")

# if the project is a subset of main cpp-ethereum project
# use same pattern for variables as Boost uses

foreach (l ${LIBS})

	string(TOUPPER ${l} L)

	find_library(Xmr_${L}_LIBRARY
		NAMES ${l}
		PATHS ${CMAKE_LIBRARY_PATH}
		PATH_SUFFIXES "/src/${l}" "/src/" "/external/db_drivers/lib${l}" "/lib" "/src/crypto" "/src/crypto/wallet" "/contrib/epee/src" "/external/easylogging++/" "/external/${l}" "external/miniupnp/miniupnpc"
		NO_DEFAULT_PATH
	)

	set(Xmr_${L}_LIBRARIES ${Xmr_${L}_LIBRARY})

	message(STATUS FindMonero " Xmr_${L}_LIBRARIES ${Xmr_${L}_LIBRARY}")

	if(NOT "${Xmr_${L}_LIBRARIES}" STREQUAL "${Xmr_${L}_LIBRARY-NOTFOUND}")
	  add_library(${l} STATIC IMPORTED)
	  set_property(TARGET ${l} PROPERTY IMPORTED_LOCATION ${Xmr_${L}_LIBRARIES})
	endif()

endforeach()

if (EXISTS ${MONERO_BUILD_DIR}/src/ringct/libringct_basic.a)
	message(STATUS FindMonero " found libringct_basic.a")
	add_library(ringct_basic STATIC IMPORTED)
	set_property(TARGET ringct_basic
			PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/ringct/libringct_basic.a)
endif()

if (EXISTS ${MONERO_BUILD_DIR}/src/cryptonote_basic/libcryptonote_format_utils_basic.a)
        message(STATUS FindMonero " found libcryptonote_format_utils_basic.a")
        add_library(cryptonote_format_utils_basic STATIC IMPORTED)
        set_property(TARGET cryptonote_format_utils_basic
                PROPERTY IMPORTED_LOCATION ${MONERO_BUILD_DIR}/src/cryptonote_basic/libcryptonote_format_utils_basic.a)
endif()


message(STATUS ${MONERO_SOURCE_DIR}/build)

# include monero headers
include_directories(
		${MONERO_SOURCE_DIR}/src
                ${MONERO_SOURCE_DIR}/src/crypto
                ${MONERO_SOURCE_DIR}/src/crypto/wallet
		${MONERO_SOURCE_DIR}/external
		${MONERO_SOURCE_DIR}/external/randomx/src
		${MONERO_SOURCE_DIR}/build
		${MONERO_SOURCE_DIR}/external/easylogging++
		${MONERO_SOURCE_DIR}/contrib/epee/include
                ${MONERO_SOURCE_DIR}/external/db_drivers/liblmdb
                ${MONERO_SOURCE_DIR}/generated_include/crypto/wallet)
