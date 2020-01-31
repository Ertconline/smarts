// ertc

#include "ertc.hpp"
#include <numeric>
#include <string>
#include <ertc.nft.hpp>
using namespace std::literals;

namespace ertc {

   bool validate_coordinates(const std::vector<point>& coords) {
      return coords.size() >= 3;
   }

   ertc::ertc(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds)
   : contract(receiver, code, ds),
     validations(receiver, receiver.value),
     parameters(receiver, receiver.value),
     current_validation(receiver, receiver.value)
   {}

   void ertc::create(eosio::name creator, uint64_t id, const std::vector<point>& coords, int64_t amount) {
      require_auth(creator);

      auto it = validations.find(id);
      eosio::check(it == validations.end(), "validation with such id already exists");

      eosio::check(validate_coordinates(coords), "coordinates are wrong");
      eosio::check(amount > 0, "zero emission amount");

      validations.emplace(_self, [&](auto &fields) {
         fields.id = id;
         fields.coordinates = coords;
         fields.amount = amount;
         fields.creator = creator;
         fields.timestamp = eosio::time_point_sec{eosio::current_time_point()};
         fields.state = validation::waiting;
      });
   }

   void ertc::change(uint64_t id, const validation& object) {
      require_auth(_self);

      auto it = validations.find(id);
      eosio::check(it != validations.end(), "validation does not exist");
      eosio::check(id == it->id, "ids do not match");

      validations.modify(it, _self, [&](auto &fields) {
         fields = object;
      });
   }

   void ertc::approve(uint64_t id) {
      require_auth(_self);

      auto it = validations.find(id);
      eosio::check(it != validations.end(), "validation does not exist");
      eosio::check(it->state == validation::waiting, "wrong validation state");

      validations.modify(it, _self, [&](auto &fields) {
         fields.state = validation::validated;
      });
   }

   void ertc::preissue(uint64_t id) {
     require_auth(_self);

     currentstate current;
     if (current_validation.exists()) {
       current = current_validation.get();
       eosio::check(current.id != id, "validation id already preissued");
       eosio::check(current.state == validation::completed || current.state == validation::canceled, "pending validation is not complete");
     }

     auto it = validations.find(id);
     eosio::check(it != validations.end(), "validation does not exist");
     eosio::check(it->state == validation::validated, "wrong validation state");

     current = {id, 0, it->state};
     current_validation.set(current, _self);

     auto params = parameters.get_or_create(_self, DEFAULT_PARAMS);
     auto ext_sym = params.fund_symbol;
     nft::account_index accounts( ext_sym.get_contract(), _self.value );
     auto acc_it = accounts.find( ext_sym.get_symbol().code().raw() );
     if (acc_it != accounts.end())
       eosio::check(acc_it->balance.amount == 0, "contract balance must be zero");
   }

   void ertc::issue(uint64_t id, int64_t amount, const std::vector<point>& points) {
      require_auth(_self);

      auto it = validations.find(id);
      eosio::check(it != validations.end(), "validation does not exist");
      eosio::check(it->state == validation::validated, "wrong validation state");

      auto current = current_validation.get();
      eosio::check(current.id == id, "validation id was not preissued");
      eosio::check(current.issued < it->amount, "validation already fully issued");

      /*size_t points_size = std::accumulate(points.begin(), points.end(), 0ull, [](const auto& acc, const auto& elem){
        return acc + points_range_length(elem);
      });*/
      size_t points_size = points.size();
      eosio::check(amount == points_size, "issue amount and points mismatch");
      eosio::check(amount <= it->amount - current.issued, "too big issue amount");

      // issue tokens
      auto params = parameters.get_or_create(_self, DEFAULT_PARAMS);

      eosio::action( eosio::permission_level{ _self, "active"_n},
                     params.fund_symbol.get_contract(),
                     "issue"_n,
                     std::make_tuple(_self, eosio::asset{points_size, params.fund_symbol.get_symbol()}, points, it->id, ""s)
                  ).send();

      current.issued += amount;
      current_validation.set(current, _self);
   }

   void ertc::payout(uint64_t id) {
     require_auth(_self);

     auto it = validations.find(id);
     eosio::check(it != validations.end(), "validation does not exist");
     eosio::check(it->state == validation::validated, "wrong validation state");

     auto current = current_validation.get();
     eosio::check(current.id == id, "validation id is not pending");
     eosio::check(current.issued == it->amount, "validation is not fully issued");

     auto params = parameters.get_or_create(_self, DEFAULT_PARAMS);
     int64_t fund_cut = it->amount * params.fund_share / 100;
     int64_t creator_cut = it->amount - fund_cut;

     eosio::action( eosio::permission_level{ _self, "active"_n},
                    params.fund_symbol.get_contract(),
                    "transfer"_n,
                    std::make_tuple(_self, params.fund_account, eosio::asset{fund_cut, params.fund_symbol.get_symbol()}, ""s)
                 ).send();

     eosio::action( eosio::permission_level{ _self, "active"_n},
                    params.fund_symbol.get_contract(),
                    "transfer"_n,
                    std::make_tuple(_self, it->creator, eosio::asset{creator_cut, params.fund_symbol.get_symbol()}, ""s)
                 ).send();

     validations.modify(it, _self, [&](auto &fields) {
        fields.state = validation::completed;
     });

     current.state = validation::completed;
     current_validation.set(current, _self);
   }

   void ertc::newshare(uint8_t value) {
      require_auth(_self);

      auto result = parameters.get_or_create(_self, DEFAULT_PARAMS);
      result.fund_share = value;

      parameters.set(result, _self);
   }

   void ertc::cancel(uint64_t id) {
     require_auth(_self);

     auto it = validations.find(id);
     eosio::check(it != validations.end(), "validation does not exist");
     eosio::check(it->state != validation::completed, "cannot cancel completed validation");

     validations.modify(it, _self, [&](auto &fields) {
        fields.state = validation::canceled;
     });

     currentstate current;
     if (current_validation.exists()) {
       current = current_validation.get();
       if (current.id == id) {
         if (current.issued > 0) {
           current.state = validation::canceled;
           current_validation.set(current, _self);
         } else {
           current_validation.remove();
           validations.erase(it);
         }
       }
     }
   }

}
