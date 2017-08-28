file(GLOB tmp *.cc)
set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} ${tmp})
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} PARENT_SCOPE)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

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
