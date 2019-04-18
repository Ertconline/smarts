cmake_minimum_required(VERSION 3.5)
project(ertc VERSION 1.0.0)

find_package(eosio.cdt)

add_contract( ertc ertc ${CMAKE_CURRENT_SOURCE_DIR}/ertc.cpp )

target_include_directories(ertc.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../ertc.nft)

set_target_properties(ertc.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
