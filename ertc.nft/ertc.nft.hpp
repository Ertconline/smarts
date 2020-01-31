#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <prange.hpp>

namespace ertc {

using namespace eosio;
using std::string;
using std::vector;
typedef uint128_t uuid;

class [[eosio::contract("ertc.nft")]] nft : public eosio::contract {
public:
   using contract::contract;
   nft( name receiver, name code, datastream<const char*> ds)
   : contract(receiver, code, ds), tokens(receiver, receiver.value) {}

   [[eosio::action]]
   void create(name issuer, std::string symbol);

   [[eosio::action]]
   void issue( name to,
               asset quantity,
               vector<point> coords,
               uint64_t validation,
               string memo);

   [[eosio::action]]
   void transferid( name from,
                    name to,
                    id_type id,
                    string memo);

   [[eosio::action]]
   void transfer( name from,
                  name to,
                  asset quantity,
                  string memo);

   [[eosio::action]]
   void open(name account, symbol sym);

   struct [[eosio::table]] account {
      asset balance;
      interval_set tokens;

      uint64_t primary_key() const { return balance.symbol.code().raw(); }
   };


   struct [[eosio::table]] stats {
      asset supply;
      name issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
      uint64_t get_issuer() const { return issuer.value; }
   };

   struct [[eosio::table]] token {
      id_type id;          // Unique 64 bit identifier,
      point coords;
      asset value;         // token value (1 SYS)
      uint64_t validation;	 // token validation id

      id_type   primary_key() const { return id; }
      point     get_coords()  const { return coords; }
      asset     get_value()   const { return value; }
      uint64_t  get_symbol()  const { return value.symbol.code().raw(); }
      uint64_t  get_validation() const { return validation; }
      uint128_t get_coords_id() const { return to_coords_id(coords); }

      // generated token global uuid based on token id and
      // contract name, passed in the argument
      uuid get_global_id(name self) const {
        uint128_t self_128 = static_cast<uint128_t>(self.value);
        uint128_t id_128 = static_cast<uint128_t>(id);
        uint128_t res = (self_128 << 64) | (id_128);
        return res;
      }

      string get_unique_name() const {
        string unique_name = std::to_string(validation) + "#" + std::to_string(id);
        return unique_name;
      }

      static uint128_t to_coords_id(const point& coords) {
        uint128_t lat_128 = static_cast<uint128_t>(coords.latitude);
        uint128_t long_128 = static_cast<uint128_t>(coords.longitude);
        uint128_t res = (lat_128 << 64) | (long_128);
        return res;
      }
   };

  using account_index = eosio::multi_index<"accounts"_n, account>;

  using currency_index = eosio::multi_index<"stat"_n, stats,
                         indexed_by< "byissuer"_n, const_mem_fun< stats, uint64_t, &stats::get_issuer> > >;

  using token_index = eosio::multi_index<"token"_n, token,
                       indexed_by< "byvalidation"_n, const_mem_fun< token, uint64_t, &token::get_validation> >,
                       indexed_by< "bycoords"_n, const_mem_fun< token, uint128_t, &token::get_coords_id> > >;

private:
   token_index tokens;

   id_type mint(name owner, asset value, point coords, uint64_t validation);

   interval_set sub_balance(name owner, asset value);
   void add_balance(name owner, asset value, const interval_set& ids );
   void sub_id( name owner, asset value, id_type id );
   void sub_supply(asset quantity);
   void add_supply(asset quantity);
};

}
