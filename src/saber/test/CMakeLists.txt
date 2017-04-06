add_executable(server_test server_test.cc ${saber_server_srcs})
add_executable(multi_server_test multi_server_test.cc ${saber_server_srcs})
target_link_libraries(server_test ${Saber_LINKER_LIBS})
target_link_libraries(multi_server_test ${Saber_LINKER_LIBS})

