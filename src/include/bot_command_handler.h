#pragma once

#include <dpp/dpp.h>

class bot_command_handler {
public:
    bot_command_handler(dpp::cluster& bot);
    void handle(const dpp::slashcommand_t& event);

private:
    dpp::cluster& bot;

    void handle_balance(const dpp::slashcommand_t& event);
    void handle_dice(const dpp::slashcommand_t& event);
    void handle_rps(const dpp::slashcommand_t& event);
    void handle_roulette(const dpp::slashcommand_t& event);
    void handle_guess(const dpp::slashcommand_t& event);
    void handle_market(const dpp::slashcommand_t& event);
    void handle_buy(const dpp::slashcommand_t& event);
    void handle_richest(const dpp::slashcommand_t& event);
    void handle_work(const dpp::slashcommand_t& event);
    void handle_daily(const dpp::slashcommand_t& event);
};
