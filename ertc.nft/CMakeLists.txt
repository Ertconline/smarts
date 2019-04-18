cmake_minimum_required(VERSION 3.5)
project(ertc.nft VERSION 1.0.0)

find_package(eosio.cdt)

add_contract( ertc.nft ertc.nft ertc.nft.cpp )

set_target_properties(ertc.nft.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
