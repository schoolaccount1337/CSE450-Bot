#include "config.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <json/json.h>

Config& Config::getInstance() {
	static Config instance;
	return instance;
}

void Config::reload(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open config file: " + filename);
	}

	Json::Value root;
	Json::CharReaderBuilder readerBuilder;
	std::string errs;

	if (!Json::parseFromStream(readerBuilder, file, &root, &errs)) {
		throw std::runtime_error("Error parsing config file: " + errs);
	}

	// Clear existing feeds
	rss_feeds_.clear();

	// Load RSS feeds
	const auto& rss_feeds = root["rss_feeds"];
	for (const auto& feed : rss_feeds) {
		RSSFeedConfig feedConfig;
		feedConfig.feed_url = feed["feed_url"].asString();
		feedConfig.check_interval = feed["check_interval"].asInt();
		feedConfig.discord_channel_id = feed["discord_channel_id"].asString();
		feedConfig.ping_role_id = feed["ping_role_id"].asString();
		rss_feeds_.push_back(feedConfig);
	}

	// Load Canvas updates
	const auto& canvas = root["canvas_updates"];
	canvas_updates_.api_url = canvas["api_url"].asString();
	canvas_updates_.course_id = canvas["course_id"].asString();
	canvas_updates_.check_interval = canvas["check_interval"].asInt();
	canvas_updates_.discord_channel_id = canvas["discord_channel_id"].asString();
	canvas_updates_.ping_role_id = canvas["ping_role_id"].asString();
}

void Config::load(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open config file: " + filename);
	}

	Json::Value root;
	Json::CharReaderBuilder readerBuilder;
	std::string errs;

	if (!Json::parseFromStream(readerBuilder, file, &root, &errs)) {
		throw std::runtime_error("Error parsing config file: " + errs);
	}

	// Load RSS feeds
	const auto& rss_feeds = root["rss_feeds"];
	for (const auto& feed : rss_feeds) {
		RSSFeedConfig feedConfig;
		feedConfig.feed_url = feed["feed_url"].asString();
		feedConfig.check_interval = feed["check_interval"].asInt();
		feedConfig.discord_channel_id = feed["discord_channel_id"].asString();
		feedConfig.ping_role_id = feed["ping_role_id"].asString();
		rss_feeds_.push_back(feedConfig);
	}

	// Load Canvas updates
	const auto& canvas = root["canvas_updates"];
	canvas_updates_.api_url = canvas["api_url"].asString();
	canvas_updates_.course_id = canvas["course_id"].asString();
	canvas_updates_.check_interval = canvas["check_interval"].asInt();
	canvas_updates_.discord_channel_id = canvas["discord_channel_id"].asString();
	canvas_updates_.ping_role_id = canvas["ping_role_id"].asString();
}

const std::vector<RSSFeedConfig>& Config::getRSSFeeds() const noexcept {
	return rss_feeds_;
}

CanvasConfig& Config::getCanvasConfig() noexcept {
	return canvas_updates_;
}