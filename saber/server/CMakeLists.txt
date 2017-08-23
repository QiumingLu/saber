file(GLOB tmp *.cc)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(tmp "${CMAKE_CURRENT_SOURCE_DIR}/data_node.cc")
exclude(Saber_SERVER_SRCS "${Saber_SERVER_SRCS}" ${tmp})

set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

set(
  Saber_SERVER_HEADERS
  saber_server.h
  server_options.h
  )

set(tmp "${CMAKE_CURRENT_SOURCE_DIR}/data_node.h")
exclude(Saber_SERVER_HEADERS "${Saber_SERVER_HEADERS}" ${tmp})

install(FILES ${Saber_SERVER_HEADERS} DESTINATION include/saber/server)
