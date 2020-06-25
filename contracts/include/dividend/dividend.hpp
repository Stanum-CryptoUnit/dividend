#include <eosiolib/multi_index.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.hpp>

using namespace eosio;

class [[eosio::contract]] dividend : public eosio::contract {

public:
    dividend( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds)
    {}
    [[eosio::action]] void distribute(uint64_t id, time_point_sec slice_at);
    [[eosio::action]] void pay(uint64_t distribution_id, eosio::name pay_to, eosio::asset quantity);
    

    void apply(uint64_t receiver, uint64_t code, uint64_t action);   

    static void create_distribution(eosio::asset quantity);

    static constexpr eosio::name _self = "dividend"_n;
    static constexpr eosio::name _reserve = "reserve"_n;
    static constexpr eosio::name _findir = "findir"_n;
    static constexpr eosio::name _payer = "payer"_n;
    
    static constexpr eosio::name _token_contract = "eosio.token"_n;
    static constexpr eosio::symbol _op_symbol     = eosio::symbol(eosio::symbol_code("USDU"), 2);



    struct [[eosio::table]] distribution {
      uint64_t id;

      eosio::time_point_sec slice_at;
      eosio::time_point_sec last_pay_time;

      eosio::asset total_asset_on_pay;
      eosio::asset total_asset_payed;
      

      uint64_t primary_key() const {return id;}
      
      EOSLIB_SERIALIZE(distribution, (id)(slice_at)(last_pay_time)(total_asset_on_pay)(total_asset_payed))
    };

    typedef eosio::multi_index<"distribution"_n, distribution> distribution_index;

 
};
