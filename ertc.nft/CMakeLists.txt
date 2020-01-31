cmake_minimum_required(VERSION 3.5)
project(ertc.nft VERSION 1.0.0)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( ertc.nft ertc.nft ertc.nft.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../prange/prange.cpp)

target_include_directories(ertc.nft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../prange)
target_include_directories( ertc.nft PUBLIC /usr/include )
# set_target_properties(ertc.nft PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
