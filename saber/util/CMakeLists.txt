file(GLOB tmp *.cc)
set(Saber_SRCS ${Saber_SRCS} ${tmp})
set(Saber_SRCS ${Saber_SRCS} PARENT_SCOPE)

set(
  Saber_UTIL_HEADERS
  crc32c.h
  hash.h
  logging.h
  mutex.h
  mutexlock.h
  runloop.h
  runloop_thread.h
  thread.h
  timeops.h
  )

install(FILES ${Saber_UTIL_HEADERS} DESTINATION include/saber/util)

if (BUILD_TESTS) 
  add_subdirectory(tests)
endif()
