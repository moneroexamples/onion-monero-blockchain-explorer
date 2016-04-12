#
# Utility macros for copying files
#
# Last update: 9th January 2013
#

#
# Create a target for copying files.
#
if (NOT TARGET copy_files)
    add_custom_target(copy_files ALL)
endif()

#
# Create a variable to keep track of the number of copy files targets created.
#
if (NOT copy_target_count)
    set(copy_target_count 0)
endif (NOT copy_target_count)
set(copy_target_count ${copy_target_count} CACHE INTERNAL "" FORCE)

#-------------------------------------------------------------------------------
# Macro: COPY_FILES
#-------------------------------------------------------------------------------
#
# Description:
#   Adds a command to the build target 'copy_files' which copies files matching
#   the specifeid globbing expression from the specified source directory to
#   the specified destination directory.  
#
# Usage:
#   COPY_FILES(SRC_DIR GLOB_PAT DST_DIR)
#
# Arguments:
#   - SRC_DIR   : The source directory containging files to be copied.
#   - GLOB_PAT  : globbing expression used to match files for copying.
#   - DST_DIR   : The destination directory where files are to be copied to.
#
# Example:
#   copy_files(${CMAKE_CURRENT_SOURCE_DIR} *.dat ${CMAKE_CURRENT_BINARY_DIR})
#       Will copy all files in the current source directory with the extension 
#       '.dat' into the current binary directory.
#
macro(COPY_FILES SRC_DIR GLOB_PAT DST_DIR)
    file(GLOB file_list 
        RELATIVE ${SRC_DIR} 
        ${SRC_DIR}/${GLOB_PAT})
    math(EXPR copy_target_count '${copy_target_count}+1')
    set(copy_target_count ${copy_target_count} CACHE INTERNAL "" FORCE)
    set(target "copy_files_${copy_target_count}")
    add_custom_target(${target}) 
    foreach(filename ${file_list})
        set(src "${SRC_DIR}/${filename}")
        set(dst "${DST_DIR}/${filename}")
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst}
            COMMENT "copying: ${src} to ${dst} " VERBATIM
        )
    endforeach(filename)
    #add_dependencies(copy_files ${target})
    add_dependencies(copy_files crowxmr)
endmacro(COPY_FILES)

#-------------------------------------------------------------------------------
# Macro: COPY_FILE
#-------------------------------------------------------------------------------
#
# Description:
#   Adds a command to the build target 'copy_files' which copies the specified
#   file to the specified destination. 
#
# Usage:
#   COPY_FILE(SRC DST)
#
# Arguments:
#   - SRC      : The source filename path (the file to be copied).
#   - DST      : The destiation filename path.
#
# Example:
#   copy_file(
#        ${CMAKE_CURRENT_SOURCE_DIR}/myfile.txt 
#        ${CMAKE_CURRENT_BINARY_DIR}/myfile.txt
#   )
#
macro(COPY_FILE SRC DST)
    math(EXPR copy_target_count '${copy_target_count}+1')
    set(copy_target_count ${copy_target_count} CACHE INTERNAL "" FORCE)
    set(target "copy_files_${copy_target_count}")
    add_custom_target(${target})
    add_custom_command(TARGET ${target} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
        COMMENT "copying: ${SRC} to ${DST}" VERBATIM
    )
    add_dependencies(copy_files ${target})
endmacro(COPY_FILE)

