cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME duckprompt)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})

set(OPENSSL_USE_STATIC_LIBS TRUE)
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
find_package(OpenSSL REQUIRED)
message("OpenSSL path: '${OPENSSL_SSL_LIBRARY}'")
message("OpenSSL crypto path: '${OPENSSL_CRYPTO_LIBRARY}'")

include_directories(src/include)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(duckdb/third_party/httplib)
if(NOT TARGET yyjson)
  include_directories(duckdb/extension/json/yyjson/include)
  add_subdirectory(duckdb/extension/json/yyjson)
endif()


set(EXTENSION_SOURCES src/duckprompt_extension.cpp src/https.cpp src/chat.cpp src/quacking_duck.cpp ${YYJSON_OBJECT_FILES})
add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})
target_link_libraries(${EXTENSION_NAME}
                      ${OPENSSL_LIBRARIES}
                      )

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
