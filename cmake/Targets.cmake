macro(saber_set_saber_link)
  set(Saber_LINK saber)
endmacro()

file(GLOB_RECURSE proto_files include/saber/proto/*.proto)
saber_protobuf_generate_cpp(${proto_gen_folder} proto_SRCS proto_HDRS ${proto_files})

file(GLOB_RECURSE saber_server_SRCS
  ${proto_SRCS}
  src/saber/util/*.cc
  src/saber/net/*.cc
  src/saber/storage/.*cc
  src/saber/paxos/*.cc
  src/saber/service/*.cc
  src/saber/server/*.cc
)

file(GLOB_RECURSE saber_client_SRCS
  ${proto_SRCS}
  src/saber/util/*.cc
  src/saber/net/*.cc
  src/saber/storage/.*cc
  src/saber/service/*.cc
  src/saber/client/*.cc
)

if(CMAKE_BUILD_SHARED_LIBS)
  add_library(saber SHARED ${saber_client_SRCS})
  add_library(saber_static STATIC ${saber_client_SRCS})
  set_target_properties(saber_static PROPERTIES OUTPUT_NAME "saber")
  set_target_properties(saber PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(saber_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(saber PROPERTIES VERSION 1.0 SOVERSION 1)
  target_link_libraries(saber ${Saber_LINKER_LIBS})
  install(DIRECTORY ${Saber_INCLUDE_DIRS}/saber DESTINATION include)
  install(TARGETS saber DESTINATION lib)
  install(TARGETS saber_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
else()
  add_library(saber ${saber_client_SRCS})
  target_link_libraries(saber ${Saber_LINKER_LIBS})
  install(DIRECTORY ${Saber_INCLUDE_DIRS}/saber DESTINATION include)
  install(TARGETS saber DESTINATION lib)
endif()

add_executable(saber_server src/saber/server/saber_server_main.cc ${saber_server_SRCS})
add_executable(saber_client src/saber/client/saber_client_main.cc ${saber_client_SRCS})

target_link_libraries(saber_server ${Saber_LINKER_LIBS})
target_link_libraries(saber_client ${Saber_LINKER_LIBS})
