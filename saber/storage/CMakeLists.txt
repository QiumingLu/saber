file(GLOB tmp *.cc)
set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} ${tmp})
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(Saber_CLIENT_SRCS ${Saber_CLIENT_SRCS} PARENT_SCOPE)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

set(
  Saber_STORAGE_HEADERS
  db.h
  )

install(FILES ${Saber_STORAGE_HEADERS} DESTINATION include/saber/storage)
