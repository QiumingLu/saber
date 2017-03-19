# - Find Skywalker
#
# SKYWALKER_INCLUDE_DIRS  - List of Skywalker includes.
# SKYWALKER_LIBRARIES     - List of libraries when using Skywalker.
# SKYWALKER_FOUND         - True if Skywalker found

# Look for the header of file.
find_path(SKYWALKER_INCLUDE NAMES skywalker/node.h
                            PATHS $ENV{SKYWALKER_ROOT}/include /opt/local/include /usr/local/include /usr/include
                            DOC "Path in which the file skywalker/node.h is located.")

# Look for the library.
find_library(SKYWALKER_LIBRARY NAMES skywalker
                               PATHS $ENV{SKYWALKER_ROOT}/lib /usr/local/lib /usr/lib
                               DOC "Path to skywalker library.")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Skywalker DEFAULT_MSG SKYWALKER_INCLUDE SKYWALKER_LIBRARY)

if(SKYWALKER_FOUND)
  set(SKYWALKER_INCLUDE_DIRS ${SKYWALKER_INCLUDE})
  set(SKYWALKER_LIBRARIES ${SKYWALKER_LIBRARY})
  mark_as_advanced(SKYWALKER_INCLUDE SKYWALKER_LIBRARY)
  message(STATUS "Found Skywalker (include: ${SKYWALKER_INCLUDE_DIRS}, library: ${SKYWALKER_LIBRARIES})")
endif()
