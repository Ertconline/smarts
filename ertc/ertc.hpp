// ertc

#pragma once
#include <eosio/eosio.hpp>
#include <eosio/symbol.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <prange.hpp>

namespace ertc {

   class [[eosio::contract]] ertc : public eosio::contract {
   public:

      struct [[eosio::table]] validation {
        uint64_t id;
        std::vector<point> coordinates;
        int64_t amount;
        eosio::name creator;
        eosio::time_point_sec timestamp;
        uint8_t state;

        enum state_t : uint8_t {
          waiting = 0,
          validated,
          completed,
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
      void preissue(uint64_t id);

      [[eosio::action]]
      void issue(uint64_t id, int64_t amount, const std::vector<point>& points);

      [[eosio::action]]
      void payout(uint64_t id);

      [[eosio::action]]
      void newshare(uint8_t value);

      [[eosio::action]]
      void cancel(uint64_t id);

      struct [[eosio::table]] currentstate {
        uint64_t id;
        int64_t issued;
        uint8_t state;
      };

      struct [[eosio::table]] params {
        uint8_t fund_share;
        eosio::extended_symbol fund_symbol;
        eosio::name fund_account;
      };

   private:

      typedef eosio::multi_index<"validation"_n, validation> validation_index;
      typedef eosio::singleton<"params"_n, params> params_singleton;
      typedef eosio::singleton<"currentstate"_n, currentstate> current_singleton;

      validation_index validations;
      params_singleton parameters;
      current_singleton current_validation;

      static constexpr uint8_t POINT_DIGITS = 8;
      static constexpr params DEFAULT_PARAMS{.fund_share = 40, .fund_symbol = {{"ERTC", 0}, "ertc.nft"_n}, .fund_account = "ertc.fund"_n};
   };

}
