#pragma once

#include <string>
#include <vector>

struct RSSFeedConfig {
	std::string feed_url;
	int check_interval;
	std::string discord_channel_id;
	std::string ping_role_id;
};

struct CanvasConfig {
	std::string api_url;
	std::string course_id;
	int check_interval;
	std::string discord_channel_id;
	std::string ping_role_id;
};

class Config {
public:
	static Config& getInstance();
	void load(const std::string& filename);

	void reload(const std::string& filename);
	const std::vector<RSSFeedConfig>& getRSSFeeds() const noexcept;
	CanvasConfig& getCanvasConfig() noexcept;

private:
	Config() = default;

	std::vector<RSSFeedConfig> rss_feeds_;
	CanvasConfig canvas_updates_;
};