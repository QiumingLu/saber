if (BUILD_CLIENT_LIBS)
  add_executable(saber_client_main saber_client_main.cc mysaber.cc)
  target_link_libraries(saber_client_main ${Saber_LINKER_LIBS} ${Saber_LINK} ${SaberClient_LINK})
endif()

if (BUILD_SERVER_LIBS)
  add_executable(saber_server_main saber_server_main.cc)
  target_link_libraries(saber_server_main ${Saber_LINKER_LIBS} ${Saber_LINK} ${SaberServer_LINK})
endif()
