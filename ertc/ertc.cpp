//
// Created by jack on 2/22/19.
//

#include "ertc.hpp"
#include <ertc.nft.hpp>

#include <string>
using namespace std::literals;

namespace ertc {

   bool validate_coordinates(const std::vector<ertc::point>& coords) {
      return coords.size() >= 3;
   }

   ertc::ertc(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds)
   : contract(receiver, code, ds),
     validations(receiver, receiver.value),
     parameters(receiver, receiver.value)
   { }

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

   void ertc::issue(uint64_t id, const std::vector<point>& points) {
      require_auth(_self);

      auto it = validations.find(id);
      eosio::check(it != validations.end(), "validation does not exist");
      eosio::check(it->state == validation::validated, "wrong validation state");
      eosio::check(points.size() == it->amount, "emission amount does not equal points quantity");

      // issue tokens
      auto params = parameters.get_or_create(_self, DEFAULT_PARAMS);

      int64_t fund_cut = it->amount * params.fund_share / 100;
      int64_t creator_cut = it->amount - fund_cut;

      auto middle_it = points.begin() + fund_cut;
      std::vector<point> fund_coords(points.begin(), middle_it);
      std::vector<point> creator_coords(middle_it, points.end());

      eosio::check(creator_coords.size() == creator_cut, "creators cut mismatch");

      if (fund_cut > 0) {
         eosio::action( eosio::permission_level{ _self, "active"_n},
                        params.fund_symbol.get_contract(),
                        "issue"_n,
                        std::make_tuple(params.fund_account, eosio::asset{fund_cut, params.fund_symbol.get_symbol()}, fund_coords, it->id, ""s)
                     ).send();
      }

      eosio::action( eosio::permission_level{ _self, "active"_n},
                     params.fund_symbol.get_contract(),
                     "issue"_n,
                     std::make_tuple(it->creator, eosio::asset{creator_cut, params.fund_symbol.get_symbol()}, creator_coords, it->id, ""s)
                  ).send();

      validations.modify(it, _self, [&](auto &fields) {
         fields.state = validation::issued;
      });
   }

   void ertc::newshare(uint8_t value) {
      require_auth(_self);

      auto result = parameters.get_or_create(_self, DEFAULT_PARAMS);
      result.fund_share = value;

      parameters.set(result, _self);
   }

}
