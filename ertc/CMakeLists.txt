cmake_minimum_required(VERSION 3.5)
project(ertc VERSION 1.0.0)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( ertc ertc ertc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../prange/prange.cpp)

target_include_directories(ertc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../prange)
target_include_directories(ertc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ertc.nft)
