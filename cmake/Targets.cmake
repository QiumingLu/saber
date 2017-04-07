macro(saber_set_saber_client_link)
  set(SaberClient_LINK saber_client)
endmacro()

macro(saber_set_saber_server_link)
  set(SaberServer_LINK saber_server)
endmacro()

file(GLOB_RECURSE proto_files
  include/saber/proto/*.proto
  src/saber/proto/*.proto
)
saber_protobuf_generate_cpp(${proto_gen_folder} proto_srcs proto_hdrs ${proto_files})

set_source_files_properties(${proto_hdrs} ${proto_srcs} PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shorten-64-to-32 -Wno-deprecated-declarations -fPIC")
add_library(proto STATIC ${proto_hdrs} ${proto_srcs})
target_link_libraries(proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(proto PUBLIC ${PROTOBUF_INCLUDE_DIR})

if(CMAKE_BUILD_CLIENT_LIBS)
  install(FILES ${proto_gen_folder}/saber.pb.h DESTINATION include/saber/proto)
endif()

list(INSERT Saber_LINKER_LIBS 0 PUBLIC proto)

file(GLOB_RECURSE saber_server_srcs
  src/saber/util/*.cc
  src/saber/net/*.cc
  src/saber/storage/.*cc
  src/saber/paxos/*.cc
  src/saber/service/*.cc
  src/saber/server/*.cc
  ${proto_srcs}
)

file(GLOB_RECURSE saber_client_srcs
  src/saber/util/*.cc
  src/saber/net/*.cc
  src/saber/storage/.*cc
  src/saber/service/*.cc
  src/saber/client/*.cc
  ${proto_srcs}
)
