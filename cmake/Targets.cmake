macro(saber_set_saber_client_link)
  set(SaberClient_LINK saber_client)
endmacro()

macro(saber_set_saber_server_link)
  set(SaberServer_LINK saber_server)
endmacro()

set(Saber_LINK "")
if (BUILD_CLIENT_LIBS)
  set(Saber_LINK saber_client) 
elseif(BUILD_SERVER_LIBS)
  set(Saber_LINK saber_server)
endif()

message(STATUS "Saber_LINK=" ${Saber_LINK})  
