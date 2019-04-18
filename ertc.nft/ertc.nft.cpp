#include "ertc.nft.hpp"
using namespace eosio;

namespace ertc {

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

void nft::issue( name to, asset quantity, vector<point> coords, string tkn_name, string memo) {

	check( is_account( to ), "to account does not exist");

   // e.g. Get EOS from 3 EOS
   auto symbol = quantity.symbol;
   check( symbol.is_valid(), "invalid symbol name" );
   check( symbol.precision() == 0, "quantity must be a whole number" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

	check( tkn_name.size() <= 32, "name has more than 32 bytes" );

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
   check( quantity.amount == coords.size(), "mismatch between number of tokens and coords provided" );

   // Mint nfts
   for(auto const& point: coords) {
      mint( to, asset{1, symbol}, point, tkn_name);
   }

   // Add balance to account
   add_balance( to, quantity );
}


void nft::transferid( name	from,
                      name 	to,
                      id_type	id,
                      string	memo ) {
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

	// Ensure owner owns token
   check( send_token->owner == from, "sender does not own token with specified ID");

	const auto& st = *send_token;

	// Notify both recipients
   require_recipient( from );
   require_recipient( to );

   // Transfer NFT from sender to receiver
   tokens.modify( send_token, from, [&]( auto& token ) {
	   token.owner = to;
   });

   // Change balance of both accounts
   sub_balance( from, st.value );
   add_balance( to, st.value );
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

	//check( quantity.amount == 1, "cannot transfer quantity, not equal to 1" );

	auto symbl = tokens.get_index<"bysymbol"_n>();

	auto it = symbl.lower_bound(quantity.symbol.code().raw());

	int64_t found = 0;
	std::vector<decltype(it)> its;
	for(; it!=symbl.end() && found < quantity.amount; ++it){
		if( it->value.symbol == quantity.symbol && it->owner == from) {
			its.push_back(it);
			++found;
		}
	}

	check(found == quantity.amount, "not enough tokens found");

	// Notify both recipients
   require_recipient( from );
	require_recipient( to );

   for (const auto& token_it : its){
      tokens.modify( *token_it, from, [&]( auto& token ) {
         token.owner = to;
      });
   }

   sub_balance( from, quantity );
   add_balance( to, quantity );
}

void nft::mint( name 	owner,
                asset 	value,
                point 	coords,
		          string 	tkn_name) {
   // Check coords uniqueness
   auto coords_index = tokens.get_index<"bycoords"_n>();
   eosio::check(coords_index.find(token::to_coords_id(coords)) == coords_index.end(), "token coordinates are not unique");

   tokens.emplace( _self, [&]( auto& token ) {
      token.id = tokens.available_primary_key();
      token.coords = coords;
      token.owner = owner;
      token.value = value;
	   token.tokenName = tkn_name;
   });
}

void nft::sub_balance( name owner, asset value ) {

	account_index from_acnts( _self, owner.value );
   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void nft::add_balance( name owner, asset value ) {

	account_index to_accounts( _self, owner.value );
   auto to = to_accounts.find( value.symbol.code().raw() );
   if( to == to_accounts.end() ) {
      to_accounts.emplace( _self, [&]( auto& a ){
         a.balance = value;
      });
   } else {
      to_accounts.modify( to, _self, [&]( auto& a ) {
         a.balance += value;
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
