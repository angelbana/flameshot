# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT CPACK_GENERATOR STREQUAL "WIX")
  return()
endif()

# The installed application owns the per-user INI. Keep it out of MSI
# components so installation, repair, and removal leave settings alone.
if(NOT IS_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
  message(FATAL_ERROR "Cannot check the Windows installer staging directory")
endif()

file(GLOB_RECURSE staged_files LIST_DIRECTORIES false
     RELATIVE "${CPACK_TEMPORARY_DIRECTORY}" "${CPACK_TEMPORARY_DIRECTORY}/*")
foreach(staged_file IN LISTS staged_files)
  get_filename_component(filename "${staged_file}" NAME)
  string(TOLOWER "${filename}" filename)
  if(filename STREQUAL "flameshot.ini")
    message(FATAL_ERROR "Do not package the user configuration: ${staged_file}")
  endif()
endforeach()
