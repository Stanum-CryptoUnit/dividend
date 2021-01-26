#include "../include/dividend/dividend.hpp"

using namespace eosio;

  /*
  Действующие аккаунты: 
    _self - аккаунт контракта. Должен обладать разрешениями eosio.code для _self.
    _findir - аккаунт распределения финансового директора. Должен обладать разрешениями eosio.code для _self и только теми токенами, которые будут переданы на распределение.
    _payer - технический аккаунт, производящий распределение по срезу. 

  */
  /*  
   *  distribute(eosio::asset quantity, uint64_t slice_at_secs_since_epoch)
   *    - eosio::asset quantity - пользователь, которому принадлежит обновляемый объект начисления
   *    - uint64_t slice_at_secs_since_epoch - временная метка среза в секундах от начала эпохи
   *    
   *    Авторизация:
   *      - _findir@active

   *    Запускает распределение по указанной временной метке. 
   *
   */

  [[eosio::action]] void dividend::distribute(uint64_t id, time_point_sec slice_at, eosio::asset quantity){
    require_auth(dividend::_findir); 

    // eosio::check(slice_at > eosio::time_point_sec(now()), "Distribution time should be in the future");
    
    dividend::distribution_index distributions(dividend::_self, dividend::_self.value);
    auto distribution = distributions.find(id);

    eosio::check(distribution != distributions.end(), "Distribution is not found");
    eosio::check(quantity == distribution -> total_asset_on_pay, "Quantity is not equal distribution object quantity");

    distributions.modify(distribution, dividend::_self, [&](auto &d){
      d.slice_at = slice_at;
    });

  };




  
  /*  
   *  pay( uint64_t distribution_id, eosio::name to, eosio::asset quantity)
   *    - distribution_id - идентификатор объекта начисления
   *    - to - пользователь, которому принадлежит выводимый объект начисления
   *    - quantity - жетоны на распределении

   *    Авторизация:
   *      - _payer@active

   *    Метод выплаты из объекта распределения.
   */

  [[eosio::action]] void dividend::pay(uint64_t distribution_id, eosio::name to, eosio::asset quantity){
    require_auth(dividend::_payer);

    dividend::distribution_index distributions(dividend::_self, dividend::_self.value);
    auto distribution = distributions.find(distribution_id);
    eosio::check(distribution != distributions.end(), "Distribution object is not found");

    eosio::check(distribution -> total_asset_payed + quantity <= distribution -> total_asset_on_pay, "Distribution overflow on pay action");

    distributions.modify(distribution, dividend::_payer, [&](auto &d){
      d.last_pay_time = eosio::time_point_sec(now());
      d.total_asset_payed += quantity;
    });

    action(
        permission_level{ dividend::_self, "active"_n },
        dividend::_token_contract, "transfer"_n,
        std::make_tuple( dividend::_self, to, quantity, std::string("Pay dividends")) 
    ).send();

  };


  /*  
   *  cancell( uint64_t distribution_id)
   *    - distribution_id - идентификатор объекта начисления

   *    Авторизация:
   *      - _findir@active

   *    Метод отмены распределения и возврата токенов на баланс _findir.
   */


  [[eosio::action]] void dividend::cancel(uint64_t distribution_id){
    require_auth(dividend::_findir);

    dividend::distribution_index distributions(dividend::_self, dividend::_self.value);    

    auto distr = distributions.find(distribution_id);

    eosio::check(distr -> slice_at == eosio::time_point_sec(0), "Cannot cancel started distribution");

    action(
        permission_level{ dividend::_self, "active"_n },
        dividend::_token_contract, "transfer"_n,
        std::make_tuple( dividend::_self, _findir, distr -> total_asset_on_pay , std::string("Cancel dividend object")) 
    ).send();

    distributions.erase(distr);
  }


  /*  
   *  create_distribution(eosio::name quantity)
   *    - quantity - сумма объекта распределения в токене распределения
   *    
   *    Авторизация:
   *      - _findir@active
   *    Создает объект распределения.
   */

  void dividend::create_distribution(eosio::asset quantity){
    require_auth(dividend::_findir);

    eosio::check(quantity.symbol == dividend::_op_symbol, "Wrong symbol for distribution");

    dividend::distribution_index distributions(dividend::_self, dividend::_self.value);

    distributions.emplace(dividend::_self, [&](auto &d){
      d.id = distributions.available_primary_key();
      d.slice_at = eosio::time_point_sec(0);
      d.last_pay_time = eosio::time_point_sec(0);
      d.total_asset_on_pay = quantity;
      d.total_asset_payed = asset(0, dividend::_op_symbol);
    });

  }


extern "C" {
   
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (code == dividend::dividend::_self.value) {
          if (action == "pay"_n.value){
            execute_action(name(receiver), name(code), &dividend::pay);
          } else if (action == "distribute"_n.value){
            execute_action(name(receiver), name(code), &dividend::distribute);
          } else if (action == "cancel"_n.value){
            execute_action(name(receiver), name(code), &dividend::cancel);
          }          
        } else {
          if (action == "transfer"_n.value){
            
            struct transfer{
                eosio::name from;
                eosio::name to;
                eosio::asset quantity;
                std::string memo;
            };

            auto op = eosio::unpack_action_data<transfer>();
            
            if (op.to == dividend::_self){
              eosio::check(name(code) == dividend::_token_contract, "Wrong token contract");
              dividend::create_distribution(op.quantity);
            }

          }
        }
  };
};
