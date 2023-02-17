# This module should only be included at the very end of the root VTK
# CMakeLists.txt, and only when VTK_FRAMEWORK_BUILD is TRUE.  At the end of the
# CMake configure step, all library targets created will need to be combined
# into a single vtk.framework/vtk file.  This step needs to run last so that all
# targets have been created.  Note that there is no way to have CMake add a rule
# that follows the `install` target, otherwise we would glob up the CMake
# library installation directory instead.
#
# The parsing of all targets works by first accumulating a listing of all CMake
# directories, and then querying each directory for its targets.  If a target is
# a static library, then it must be added to the final vtk.framework/vtk.
# See https://stackoverflow.com/a/62311397 to understand why directories must be
# traversed first.

# Obtain all libraries that are part of a "regular install" to be able to
# combine them all into vtk.framework.
set(vtk_library_search_dirs "${CMAKE_CURRENT_SOURCE_DIR}")
set(vtk_all_directories)
while(vtk_library_search_dirs)
  list(POP_FRONT vtk_library_search_dirs current_directory)
  list(APPEND vtk_all_directories "${current_directory}")
  get_property(subdirectories
    DIRECTORY "${current_directory}" PROPERTY SUBDIRECTORIES)
  # NOTE: do not append empty entries, otherwise this loop never terminates.
  if (subdirectories)
    list(APPEND vtk_library_search_dirs "${subdirectories}")
  endif()
endwhile()
unset(current_directory)
unset(vtk_library_search_dirs)

# Now that we have a list of all of the different directories for the entire
# project, accumulate the list of all static library targets.
set(vtk_all_library_targets)
set(vtk_all_library_output_files)
foreach (dir IN LISTS vtk_all_directories)
  get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach (target IN LISTS dir_targets)
    get_target_property(target_type "${target}" TYPE)
    if (target_type STREQUAL "STATIC_LIBRARY")
      list(APPEND vtk_all_library_targets "${target}")
      list(APPEND
        vtk_all_library_output_files
        "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${target}>")
    endif()
  endforeach()
endforeach()
unset(vtk_all_directories)

# Now that we have the full list of all static library targets to depend on, as
# as well as a list of their output destinations, add a rule to install to
# combine everything at the end into a single file.  By depending on every
# target, this custom command will be the last to execute during build.
set(vtk_framework_path "${CMAKE_CURRENT_BINARY_DIR}/vtk.framework")
add_custom_command(
  OUTPUT "${vtk_framework_path}/vtk"
  COMMENT "Combining targets ${vtk_all_library_targets} into ${vtk_framework_path}."
  COMMAND
    ${CMAKE_COMMAND} -E rm -rf "${vtk_framework_path}"
  COMMAND
    ${CMAKE_COMMAND} -E make_directory "${vtk_framework_path}/Headers"
  COMMAND
    ${CMAKE_COMMAND} -E make_directory "${vtk_framework_path}/Resources"
  COMMAND
    libtool -static -o "${vtk_framework_path}/vtk_lib" ${vtk_all_library_output_files}
  COMMAND
    lipo -create "${vtk_framework_path}/vtk_lib" -output "${vtk_framework_path}/vtk"
  COMMAND
    ${CMAKE_COMMAND} -E rm -f "${vtk_framework_path}/vtk_lib"
  DEPENDS ${vtk_all_library_targets}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_target(vtk-framework ALL DEPENDS "${vtk_framework_path}/vtk")
install(
  FILES "${vtk_framework_path}/vtk"
  DESTINATION "vtk.framework")
