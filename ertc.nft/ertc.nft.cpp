#include <numeric>
#include "ertc.nft.hpp"

using namespace eosio;

namespace ertc {

void nft::open(name account, symbol sym) {
  require_auth( account );
  add_balance(account, asset{0, sym}, {});
}

void nft::create( name issuer, std::string sym ) {
  require_auth( _self );

  // Check if issuer account exists
  check( is_account( issuer ), "issuer account does not exist");

   // Valid symbol
   asset supply(0, symbol( symbol_code( sym.c_str() ), 0) );

   auto symbol = supply.symbol;
   check( symbol.is_valid(), "invalid symbol name" );

   // Check if currency with symbol already exists
  auto symbol_name = symbol.code().raw();
   currency_index currency_table( _self, symbol_name );
   auto existing_currency = currency_table.find( symbol_name );
   check( existing_currency == currency_table.end(), "token with symbol already exists" );

   // Create new currency
   currency_table.emplace( _self, [&]( auto& currency ) {
      currency.supply = supply;
      currency.issuer = issuer;
   });
}

void nft::issue( name to, asset quantity, vector<point> coords, uint64_t validation, string memo) {
   check( is_account( to ), "to account does not exist");

   // e.g. Get EOS from 3 EOS
   auto symbol = quantity.symbol;
   check( symbol.is_valid(), "invalid symbol name" );
   check( symbol.precision() == 0, "quantity must be a integer" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   // Ensure currency has been created
   auto symbol_name = symbol.code().raw();
   currency_index currency_table( _self, symbol_name );
   auto existing_currency = currency_table.find( symbol_name );
   check( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );
   const auto& st = *existing_currency;

   // Ensure have issuer authorization and valid quantity
   require_auth( st.issuer );
   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must issue positive quantity of NFT" );
   check( symbol == st.supply.symbol, "symbol precision mismatch" );

   // Increase supply
   add_supply( quantity );

   // Check that number of tokens matches coords size
   size_t coords_size = coords.size();
   check( quantity.amount == coords_size, "mismatch between number of tokens and coords provided" );

   id_pair ids;
   ids.first = tokens.available_primary_key();
   // Mint nfts
   for (const auto& pt: coords)
     mint( to, asset{1, symbol}, pt, validation);
   ids.second = tokens.available_primary_key() - 1;
   // Add balance to account
   add_balance( to, quantity, {ids} );
}

void nft::transferid( name	from,
                      name 	to,
                      id_type	id,
                      string	memo ) {
   check(false, "transferid action is disabled");
   // Ensure authorized to send from account
   check( from != to, "cannot transfer to self" );
   require_auth( from );

   // Ensure 'to' account exists
   check( is_account( to ), "to account does not exist");

  // Check memo size and print
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   // Ensure token ID exists
   auto send_token = tokens.find( id );
   check( send_token != tokens.end(), "token with specified ID does not exist" );
   const auto& st = *send_token;

   // Notify both recipients
   require_recipient( from );
   require_recipient( to );

   // Change balance of both accounts
   sub_id( from, st.value, id );
   add_balance( to, st.value, {{id,id}} );
}

void nft::transfer( name 	from,
                    name 	to,
                    asset	quantity,
                    string	memo ) {
   // Ensure authorized to send from account
   check( from != to, "cannot transfer to self" );
   require_auth( from );

   // Ensure 'to' account exists
   check( is_account( to ), "to account does not exist");

   // Check memo size and print
   check( memo.size() <= 256, "memo has more than 256 bytes" );

  // Notify both recipients
  require_recipient( from );
  require_recipient( to );

  auto token_ids = sub_balance( from, quantity );
  add_balance( to, quantity, token_ids );
}

id_type nft::mint( name 	 owner,
                asset 	 value,
                point 	 coords,
            uint64_t validation) {
   // Check coords uniqueness
   auto coords_index = tokens.get_index<"bycoords"_n>();
   eosio::check(coords_index.find(token::to_coords_id(coords)) == coords_index.end(), "token coordinates are not unique");

   auto it = tokens.emplace( _self, [&]( auto& token ) {
      token.id = tokens.available_primary_key();
      token.coords = coords;
      token.value = value;
      token.validation = validation;
   });

   return it->id;
}

void nft::sub_id( name owner, asset value, id_type id ) {
   account_index from_acnts( _self, owner.value );
   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( value.amount == 1, "can only substract one id");
   check( from.balance.amount >= value.amount, "overdrawn balance" );
   //check( from.balance.amount == from.tokens.size(), "balance and tokens mismatch" );

   auto it = std::lower_bound(from.tokens.begin(), from.tokens.end(), id, [](const auto& a, const auto& b){
     return a.second < b;
   });
   check( it != from.tokens.end(), "does not own specified token id");

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
          a.tokens.erase(it);
      });
   }
}

interval_set nft::sub_balance( name owner, asset value ) {

   account_index from_acnts( _self, owner.value );
   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );
   //check( from.balance.amount == from.tokens.size(), "balance and tokens mismatch" );

   interval_set result;
   if( from.balance.amount == value.amount ) {
      result = from.tokens;
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
          result = substract_amount(a.tokens, value.amount);
      });
   }

   return result;
}

void nft::add_balance( name owner, asset value, const interval_set& ids ) {

   account_index to_accounts( _self, owner.value );
   auto to = to_accounts.find( value.symbol.code().raw() );
   if( to == to_accounts.end() ) {
      to = to_accounts.emplace( _self, [&]( auto& a ){
         a.balance = value;
         a.tokens = ids;
      });
   } else {
      to_accounts.modify( to, _self, [&]( auto& a ) {
         a.balance += value;
         merge_sets(a.tokens, ids.begin(), ids.end());
      });
   }
}

void nft::sub_supply( asset quantity ) {

   auto symbol_name = quantity.symbol.code().raw();
   currency_index currency_table( _self, symbol_name );
   auto current_currency = currency_table.find( symbol_name );

   currency_table.modify( current_currency, _self, [&]( auto& currency ) {
      currency.supply -= quantity;
   });
}

void nft::add_supply( asset quantity ) {

   auto symbol_name = quantity.symbol.code().raw();
   currency_index currency_table( _self, symbol_name );
   auto current_currency = currency_table.find( symbol_name );

   currency_table.modify( current_currency, name(0), [&]( auto& currency ) {
      currency.supply += quantity;
   });
}

}
