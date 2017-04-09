add_executable(saber_server_main saber_server_main.cc ${saber_server_srcs})
add_executable(saber_client_main saber_client_main.cc mysaber.cc to_string.cc ${saber_client_srcs})

target_link_libraries(saber_server_main ${Saber_LINKER_LIBS})
target_link_libraries(saber_client_main ${Saber_LINKER_LIBS})
