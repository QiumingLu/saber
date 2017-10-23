file(GLOB tmp *.cc)
set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} ${tmp})

set(Saber_SERVER_SRCS ${Saber_SERVER_SRCS} PARENT_SCOPE)

set(
  Saber_SERVER_HEADERS
  saber_server.h
  server_options.h
  )

install(FILES ${Saber_SERVER_HEADERS} DESTINATION include/saber/server)

if(BUILD_SHARED_LIBS AND BUILD_SERVER_LIBS)
  add_library(saber_server SHARED ${Saber_SERVER_SRCS})
  add_library(saber_server_static STATIC ${Saber_SERVER_SRCS})
  set_target_properties(saber_server_static PROPERTIES OUTPUT_NAME "saber_server")
  set_target_properties(saber_server PROPERTIES CLEAN_DIRECT_OUTPUT ${SABER_VERSION_MAJOR})
  set_target_properties(saber_server_static PROPERTIES CLEAN_DIRECT_OUTPUT ${SABER_VERSION_MAJOR})
  set_target_properties(saber_server PROPERTIES VERSION ${SABER_VERSION_STRING} SOVERSION ${SABER_VERSION_MAJOR})
  target_link_libraries(saber_server ${Saber_LINKER_LIBS} ${Saber_LINK})
  install(TARGETS saber_server DESTINATION lib)
  install(TARGETS saber_server_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
elseif(BUILD_SERVER_LIBS)
  add_library(saber_server ${Saber_SERVER_SRCS})
  target_link_libraries(saber_server ${Saber_LINKER_LIBS} ${Saber_LINK})
  install(TARGETS saber_server DESTINATION lib)
endif()

