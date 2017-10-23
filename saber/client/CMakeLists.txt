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

if(BUILD_SHARED_LIBS AND BUILD_CLIENT_LIBS)
  add_library(saber_client SHARED ${Saber_CLIENT_SRCS})
  add_library(saber_client_static STATIC ${Saber_CLIENT_SRCS})
  set_target_properties(saber_client_static PROPERTIES OUTPUT_NAME "saber_client")
  set_target_properties(saber_client PROPERTIES CLEAN_DIRECT_OUTPUT ${SABER_VERSION_MAJOR})
  set_target_properties(saber_client_static PROPERTIES CLEAN_DIRECT_OUTPUT ${SABER_VERSION_MAJOR})
  set_target_properties(saber_client PROPERTIES VERSION ${SABER_VERSION_STRING} SOVERSION ${SABER_VERSION_MAJOR})
  target_link_libraries(saber_client ${Saber_LINKER_LIBS} ${Saber_LINK})
  install(TARGETS saber_client DESTINATION lib)
  install(TARGETS saber_client_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
elseif(BUILD_CLIENT_LIBS)  
  add_library(saber_client ${Saber_CLIENT_SRCS})
  target_link_libraries(saber_client ${Saber_LINKER_LIBS} ${Saber_LINK})
  install(TARGETS saber_client DESTINATION lib)
endif()
