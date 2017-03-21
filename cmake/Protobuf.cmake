find_package(Protobuf REQUIRED)
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})
list(APPEND Saber_LINKER_LIBS ${PROTOBUF_LIBRARIES})

if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
  message(STATUS "Found Protobuf Compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
else()
  message(FATAL_ERROR "Could not find Protobuf Compiler")
endif()

if(PROTOBUF_FOUND)
  message(STATUS "Found Protobuf (include: ${PROTOBUF_INCLUDE_DIR}, libraries: ${PROTOBUF_LIBRARIES})")
endif()

set(proto_gen_folder ${PROJECT_SOURCE_DIR}/include/saber/proto)
include_directories(${PROJECT_BINARY_DIR})

set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)

function(saber_protobuf_generate_cpp output_dir srcs_var hdrs_var)
  if(NOT ARGN)
    message(SEND_ERROR "Error:saber_protobuf_generate_cpp() called without any proto files")
    return()
  endif()

  if (PROTOBUF_GENERATE_CPP_APPEND_PATH)
    foreach(fil ${ARGN})
      get_filename_component(abs_fil ${fil} ABSOLUTE)
      get_filename_component(abs_path ${abs_fil} PATH)
      list(FIND _protoc_include ${abs_path} _contains_already)
      if(${_contains_already} EQUAL -1)
        list(APPEND _protoc_include -I ${abs_path})
      endif()
    endforeach()
  else()
    set(_protoc_include -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if (DEFINED PROTOBUF_IMPORT_DIRS)
    foreach(dir ${PROTOBUF_IMPORT_DIRS})
      get_filename_component(abs_path ${dir} ABSOLUTE)
      list(FIND _protoc_include ${abs_path} _contains_already)
      if (${_contains_already} EQUAL -1)
        list(APPEND _protoc_include -I ${abs_path})
      endif()
    endforeach()
  endif()

  set(${srcs_var})
  set(${hdrs_var})
  foreach(fil ${ARGN})
    get_filename_component(abs_fil ${fil} ABSOLUTE)
    get_filename_component(fil_we ${fil} NAME_WE)

    list(APPEND ${srcs_var} "${output_dir}/${fil_we}.pb.cc")
    list(APPEND ${hdrs_var} "${output_dir}/${fil_we}.pb.h")

    add_custom_command(
      OUTPUT "${output_dir}/${fil_we}.pb.cc"
             "${output_dir}/${fil_we}.pb.h"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out ${output_dir} ${_protoc_include} ${abs_fil}
      DEPENDS ${abs_fil}
      COMMENT "Running C++ protobuf protocol buffer compiler on ${fil}" VERBATIM)
  endforeach()

  set_source_files_properties(${${srcs_var}} ${${hdrs_var}} PROPERTIES GENERATED TRUE)
  set(${srcs_var} ${${srcs_var}} PARENT_SCOPE)
  set(${hdrs_var} ${${hdrs_var}} PARENT_SCOPE)
endfunction()
