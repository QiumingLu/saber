file(GLOB tmp *.cc)
set(Saber_SRCS ${Saber_SRCS} ${tmp})
set(Saber_SRCS ${Saber_SRCS} PARENT_SCOPE)

set(
  Saber_SERVICE_HEADERS
  watcher.h
  )

install(FILES ${Saber_SERVICE_HEADERS} DESTINATION include/saber/service)
