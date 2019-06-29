//
// Created by jack on 2/22/19.
//

#pragma once
#include <eosio/eosio.hpp>
#include <eosio/symbol.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

namespace ertc {

   class [[eosio::contract]] ertc : public eosio::contract {
   public:

      struct point {
        int64_t latitude;
        int64_t longitude;
      };

      struct [[eosio::table]] validation {
        uint64_t id;
        // coordinates
        std::vector<point> coordinates;
        int64_t amount;
        eosio::name creator;
        eosio::time_point_sec timestamp;
        uint8_t state;

        enum state_t : uint8_t {
          waiting = 0,
          validated,
          issued,
          canceled
        };

        uint64_t primary_key() const { return id; }
      };

      using contract::contract;
      ertc(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds);

      [[eosio::action]]
      void create(eosio::name creator, uint64_t id, const std::vector<point>& coords, int64_t amount);

      [[eosio::action]]
      void change(uint64_t id, const validation& object);

      [[eosio::action]]
      void approve(uint64_t id);

      [[eosio::action]]
      void issue(uint64_t id, const std::vector<point>& points);

      [[eosio::action]]
      void newshare(uint8_t value);

      struct [[eosio::table]] params {
        uint8_t fund_share;
        eosio::extended_symbol fund_symbol;
        eosio::name fund_account;
      };

   private:

      typedef eosio::multi_index<"validation"_n, validation> validation_index;
      typedef eosio::singleton<"params"_n, params> params_singleton;

      validation_index validations;
      params_singleton parameters;

      static constexpr uint8_t POINT_DIGITS = 8;
      static constexpr params DEFAULT_PARAMS {.fund_share = 40, .fund_symbol = {{"ERTC", 0}, "ertc.nft"_n}, .fund_account = "ertc.fund"_n};
   };

}
