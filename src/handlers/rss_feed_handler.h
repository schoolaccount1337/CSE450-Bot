#pragma once

#include <dpp/dpp.h>
#include "../config/config.h"
#include <string>
#include <vector>
#include <chrono>
#include <regex>
#include <unordered_map>
#include <algorithm>

struct FeedItem {
	std::string title;
	std::string content;
	std::string feed_url;
};

class RSSFeedHandler {
public:
	RSSFeedHandler(dpp::cluster& bot, const Config& config);
	void start();

private:
	dpp::cluster& bot_;
	const Config& config_;

	struct FeedState {
		RSSFeedConfig config;
		std::string last_item_guid;
		std::chrono::steady_clock::time_point next_check;
	};

	std::vector<FeedState> feed_states_;

	void checkFeeds(); 
	FeedItem fetchLatestItem(const std::string& feed_url);
};
