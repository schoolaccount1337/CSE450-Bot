#include "include/command_register.h"
#include <vector>

void register_global_commands(dpp::cluster& bot) {
    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            std::vector<dpp::slashcommand> commands = {
                dpp::slashcommand("balance", "Check the balance of a user", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_user, "user", "The user to check balance for", false)),

                dpp::slashcommand("coins", "Check the balance of a user (alias of /balance)", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_user, "user", "The user to check balance for", false)),

                dpp::slashcommand("richest", "See the richest user", bot.me.id),

                dpp::slashcommand("top", "See the top users (alias of /richest)", bot.me.id),

                dpp::slashcommand("dice", "Roll a dice with a specified amount or challenge a user", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_integer, "amount", "Amount to roll the dice with", true))
                    .add_option(dpp::command_option(dpp::co_user, "user", "The user to challenge", false)),

                dpp::slashcommand("work", "Do a job", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_string, "job", "Type of job to perform", true)
                        .add_choice(dpp::command_option_choice("Construction Work", "construction_work"))
                        .add_choice(dpp::command_option_choice("Office Job", "office_job"))
                        .add_choice(dpp::command_option_choice("Startup Founder", "startup_founder"))),

                dpp::slashcommand("daily", "Claim your daily reward", bot.me.id),

                dpp::slashcommand("rps", "Play Rock, Paper, Scissors", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_integer, "amount", "The amount to wager", true))
                    .add_option(dpp::command_option(dpp::co_user, "user", "The user to challenge", false)),

                dpp::slashcommand("roulette", "Play roulette with a specified amount", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_integer, "amount", "Amount to wager", true)),

                dpp::slashcommand("guess", "Play a guessing game", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_integer, "amount", "Amount to wager", true)),

                dpp::slashcommand("market", "Access the market", bot.me.id),

                dpp::slashcommand("shop", "Access the shop (alias of /market)", bot.me.id),

                dpp::slashcommand("buy", "Buy an item from the shop", bot.me.id)
                    .add_option(dpp::command_option(dpp::co_integer, "item_number", "The item number to buy", true))
            };

            bot.global_bulk_command_create(commands, [](const dpp::confirmation_callback_t& event) {
                if (event.is_error()) {
                    std::cerr << "Error registering commands: " << event.get_error().message << "\n";
                } else {
                    std::cout << "Commands registered successfully!\n";
                }
            });
        }
    });
}
