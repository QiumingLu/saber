file(GLOB tmp *.cc)
set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} ${tmp})

set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} PARENT_SCOPE)

set(
  Saber_CLIENT_HEADERS
  callbacks.h
  client_options.h
  saber.h
  server_manager.h
  )

install(FILES ${Saber_CLIENT_HEADERS} DESTINATION include/saber/client)
