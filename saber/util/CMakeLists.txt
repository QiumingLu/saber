file(GLOB tmp *.cc)
set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} ${tmp})
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} PARENT_SCOPE)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

set(
  Saber_UTIL_HEADERS
  blocking_queue.h
  crc32c.h
  concurrent_map.h
  logging.h
  mutex.h
  mutexlock.h
  runloop.h
  runloop_thread.h
  slice.h
  thread.h
  timeops.h
  )

install(FILES ${Saber_UTIL_HEADERS} DESTINATION include/saber/util)
