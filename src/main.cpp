#include <dpp/dpp.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <optional>
#include "config/config.h"
#include "include/command_register.h"
#include "include/bot_command_handler.h"
#include "handlers/rss_feed_handler.h"
#include "handlers/canvas_handler.h"

std::optional<std::string> parse_args(int argc, char* argv[]);
void display_help();



int main(int argc, char* argv[]) {
    // Parse CLI args
    auto operation = parse_args(argc, argv);
    if (!operation) {
        return 1;
    }

    // Get environment variables
	const std::string BOT_TOKEN = std::getenv("CSE450BOTTOKEN");
	const std::string CANVAS_TOKEN = std::getenv("CANVASTOKEN");
    if (BOT_TOKEN.empty() || CANVAS_TOKEN.empty()) {
        std::cerr << "Please set the appropriate environment variables. See the README for more information";
        return 1;
    }

    // Load /config/config.json
	try {
		Config& config = Config::getInstance();
		config.load("config/config.json");
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to load configuration: " << e.what() << "\n";
		return 1;
	}

    // Create bot
    dpp::cluster bot(BOT_TOKEN);
    bot.on_log(dpp::utility::cout_logger());

    // Setup slash command handler
    if (*operation == "register-on-load") {
        register_global_commands(bot);
    }

    bot_command_handler handler(bot);

    bot.on_slashcommand([&handler](const dpp::slashcommand_t& event) {
        handler.handle(event);
    });

    // Set up RSS feed handler
	RSSFeedHandler rssHandler(bot, Config::getInstance());
	rssHandler.start();

	// Set up Canvas handler
	CanvasHandler canvasHandler(bot, Config::getInstance().getCanvasConfig(), CANVAS_TOKEN);
	canvasHandler.start();

    // Ready bot
    bot.on_ready([&bot](const dpp::ready_t& event) {
        std::cout << "Bot is ready!\n";
    });

    // Start bot
    bot.start(dpp::st_wait);
    return 0;
}

void display_help() {
    std::cout << R"(
Usage: bot [options]
Options:
  --help, -h              Show this help message and exit
  --register-on-load      Register global commands when the bot starts
)";
}

std::optional<std::string> parse_args(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [options]\n";
        display_help();
        return std::nullopt;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            display_help();
            return std::nullopt;
        } else if (arg == "--register-on-load") {
            return "register-on-load";
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return std::nullopt;
        }
    }

    return std::nullopt;
}
