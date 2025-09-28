function(link_aws_static target_name)
  if(DEFINED OPENSSL_INCLUDE_DIR)
    target_include_directories(${target_name} PRIVATE ${OPENSSL_INCLUDE_DIR})
  endif()
  if(DEFINED CURL_INCLUDE_DIR)
    target_include_directories(${target_name} PRIVATE ${CURL_INCLUDE_DIR})
  endif()
  if(EXISTS "${AWS_CRT_CPP_DIR}/include")
    target_include_directories(${target_name} PRIVATE ${AWS_CRT_CPP_DIR}/include)
  endif()
  if(EXISTS "${AWS_SDK_CPP_DIR}/include")
    target_include_directories(${target_name} PRIVATE ${AWS_SDK_CPP_DIR}/include)
  endif()

  _pick_lib(_aws_core      aws-cpp-sdk-core)
  _pick_lib(_aws_s3        aws-cpp-sdk-s3)
  _pick_lib(_aws_transfer  aws-cpp-sdk-transfer)
  _pick_lib(_aws_crt_cpp   aws-crt-cpp)

  _pick_lib(_awsc_common   aws-c-common)
  _pick_lib(_awsc_sdkutils aws-c-sdkutils)
  _pick_lib(_awsc_http     aws-c-http)
  _pick_lib(_awsc_io       aws-c-io)
  _pick_lib(_awsc_comp     aws-c-compression)
  _pick_lib(_awsc_cal      aws-c-cal)
  _pick_lib(_awsc_auth     aws-c-auth)
  _pick_lib(_awsc_mqtt     aws-c-mqtt)
  _pick_lib(_awsc_es       aws-c-event-stream)
  _pick_lib(_awsc_s3       aws-c-s3)
  _pick_lib(_checksums     aws-checksums)
  _pick_lib(_s2n           s2n)

  _pick_lib(_curl          curl)
  _pick_lib(_ssl           ssl)
  _pick_lib(_crypto        crypto)
  _pick_lib(_zlib          z)

  target_link_libraries(${target_name} PRIVATE
    "-Wl,--start-group"
      ${_aws_transfer}
      ${_aws_s3}
      ${_aws_core}
      ${_aws_crt_cpp}

      ${_awsc_http}
      ${_awsc_io}
      ${_awsc_comp}
      ${_awsc_cal}
      ${_awsc_auth}
      ${_awsc_mqtt}
      ${_awsc_es}
      ${_awsc_common}
      ${_awsc_sdkutils}
      ${_awsc_s3}
      ${_checksums}

      ${_curl}
      ${_s2n}
      ${_ssl}
      ${_crypto}
      ${_zlib}
    "-Wl,--end-group"

    pthread dl m
  )
endfunction()
