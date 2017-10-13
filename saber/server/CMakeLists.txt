file(GLOB tmp *.cc)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

set(
  Saber_SERVER_HEADERS
  saber_server.h
  server_options.h
  )

install(FILES ${Saber_SERVER_HEADERS} DESTINATION include/saber/server)
