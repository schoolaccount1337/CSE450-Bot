#include "rss_feed_handler.h"
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string decodeHTMLEntities(const std::string& input) {
	static const std::unordered_map<std::string, std::string> htmlEntities = {
		{"&nbsp;", " "},
		{"&amp;", "&"},
		{"&lt;", "<"},
		{"&gt;", ">"},
		{"&quot;", "\""},
		{"&apos;", "'"},
		{"&#39;", "'"}
	};

	std::string output = input;
	for (const auto& [entity, replacement] : htmlEntities) {
		size_t pos = 0;
		while ((pos = output.find(entity, pos)) != std::string::npos) {
			output.replace(pos, entity.length(), replacement);
			pos += replacement.length();
		}
	}
	return output;
}

std::string trim(const std::string& str) {
	auto begin = str.find_first_not_of(' ');
	if (begin == std::string::npos) return "";
	auto end = str.find_last_not_of(' ');
	return str.substr(begin, end - begin + 1);
}

std::string convertHTMLToMarkdown(const std::string& input) {
	std::string output = input;

	// Handle paragraph tags <p>
	output = std::regex_replace(output, std::regex(R"(<\s*p\s*>\s*&nbsp;\s*</p\s*>)"), "\n"); // Handle <p>&nbsp;</p> as just a newline
	output = std::regex_replace(output, std::regex(R"(<\s*p\s*>|\s*<\s*/\s*p\s*>)"), "\n\n"); // Handle normal <p> as double newlines

	// Handle <br> tags to single newlines
	output = std::regex_replace(output, std::regex(R"(<\s*br\s*/?\s*>)"), "\n");

	// Handle headers <h1> to <h6>
	output = std::regex_replace(output, std::regex(R"(<\s*h1\s*>|\s*<\s*/\s*h1\s*>)"), "# ");
	output = std::regex_replace(output, std::regex(R"(<\s*h2\s*>|\s*<\s*/\s*h2\s*>)"), "## ");
	output = std::regex_replace(output, std::regex(R"(<\s*h3\s*>|\s*<\s*/\s*h3\s*>)"), "### ");
	output = std::regex_replace(output, std::regex(R"(<\s*h4\s*>|\s*<\s*/\s*h4\s*>)"), "#### ");
	output = std::regex_replace(output, std::regex(R"(<\s*h5\s*>|\s*<\s*/\s*h5\s*>)"), "##### ");
	output = std::regex_replace(output, std::regex(R"(<\s*h6\s*>|\s*<\s*/\s*h6\s*>)"), "###### ");

	// Handle bold tags <b> or <strong> to Markdown **bold**
	output = std::regex_replace(output, std::regex(R"(<\s*b\s*>|<\s*strong\s*>)"), "**");
	output = std::regex_replace(output, std::regex(R"(<\s*/\s*b\s*>|<\s*/\s*strong\s*>)"), "**");

	// Handle italic tags <i> or <em> to Markdown *italics*
	output = std::regex_replace(output, std::regex(R"(<\s*i\s*>|<\s*em\s*>)"), "*");
	output = std::regex_replace(output, std::regex(R"(<\s*/\s*i\s*>|<\s*/\s*em\s*>)"), "*");

	// Handle inline code tags <code> to Markdown `code`
	output = std::regex_replace(output, std::regex(R"(<\s*code\s*>|\s*<\s*/\s*code\s*>)"), "`");

	// Handle blockquote <blockquote> to Markdown >
	output = std::regex_replace(output, std::regex(R"(<\s*blockquote\s*>|\s*<\s*/\s*blockquote\s*>)"), "\n> ");

	// Handle unordered list items <ul><li> to Markdown - list items
	output = std::regex_replace(output, std::regex(R"(<\s*li\s*>|\s*<\s*/\s*li\s*>)"), "\n- ");
	output = std::regex_replace(output, std::regex(R"(<\s*/\s*ul\s*>)"), "\n");

	// Handle <a href="url">link text</a> to Markdown [link text](url)
	output = std::regex_replace(output, std::regex(R"(<\s*a\s+href\s*=\s*['"]([^'"]+)['"]\s*>([^<]+)<\s*/\s*a\s*>)"),
		"[$2]($1)");

	// Decode common HTML entities
	output = decodeHTMLEntities(output);

	// Remove any other remaining HTML tags
	output = std::regex_replace(output, std::regex(R"(<[^>]*>)"), "");

	// Trim trailing/leading whitespace
	output = trim(output);

	return output;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
	size_t newLength = size * nmemb;
	try {
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc& e) {
		return 0;
	}
	return newLength;
}

RSSFeedHandler::RSSFeedHandler(dpp::cluster& bot, const Config& config)
	: bot_(bot), config_(config) {
	for (const auto& feed_config : config_.getRSSFeeds()) {
		feed_states_.emplace_back(FeedState{
			feed_config,
			"",
			std::chrono::steady_clock::now()
			});
	}
}

void RSSFeedHandler::start() {
	std::thread([this]() {
		while (true) {
			checkFeeds();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		}).detach();
}

void RSSFeedHandler::checkFeeds() {
	const auto now = std::chrono::steady_clock::now();
	for (auto& feed_state : feed_states_) {
		if (now >= feed_state.next_check) {
			FeedItem latest_item = fetchLatestItem(feed_state.config.feed_url);

			if (!latest_item.title.empty() && latest_item.title != feed_state.last_item_guid) {
				feed_state.last_item_guid = latest_item.title;

				std::string message_content = "📢 **" + latest_item.title +
					"**\n\n---" + latest_item.content + "---\n\nSee full announcement here: " + latest_item.feed_url +
					"\n<@&" + feed_state.config.ping_role_id + ">";

				dpp::message msg(feed_state.config.discord_channel_id, message_content);
				msg.allowed_mentions.parse_roles = true;
				bot_.message_create(msg);
			}

			feed_state.next_check = now + std::chrono::seconds(feed_state.config.check_interval);
		}
	}
}

// Fetch using libcurl and libxml2
FeedItem RSSFeedHandler::fetchLatestItem(const std::string& feed_url) {
	CURL* curl;
	CURLcode res;
	std::string rss_content;

	curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize curl.\n";
		return { "", "", feed_url };
	}

	curl_easy_setopt(curl, CURLOPT_URL, feed_url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rss_content);
	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		std::cerr << "Failed to fetch RSS feed: " << curl_easy_strerror(res) << "\n";
		curl_easy_cleanup(curl);
		return { "", "", feed_url };
	}

	// Cleanup
	curl_easy_cleanup(curl);

	// Parse w/ libxml2
	xmlDoc* doc = xmlReadMemory(rss_content.c_str(), rss_content.size(), "noname.xml", NULL, 0);
	if (doc == nullptr) {
		std::cerr << "Failed to parse RSS feed.\n";
		return { "", "", feed_url };
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);
	xmlNode* entry = nullptr;

	// First <entry> element
	for (xmlNode* node = root_element->children; node; node = node->next) {
		if (xmlStrEqual(node->name, BAD_CAST "entry")) {
			entry = node;
			break;
		}
	}

	std::string latest_id;
	std::string latest_title;
	std::string latest_content;

	if (entry) {
		// Extract the <id>, <title>, <content>/<summary>
		for (xmlNode* child = entry->children; child; child = child->next) {
			if (xmlStrEqual(child->name, BAD_CAST "id")) {
				xmlChar* content = xmlNodeGetContent(child);
				latest_id = std::string((char*)content);
				xmlFree(content);
			}
			else if (xmlStrEqual(child->name, BAD_CAST "title")) {
				xmlChar* content = xmlNodeGetContent(child);
				latest_title = std::string((char*)content);
				xmlFree(content);
			}
			else if (xmlStrEqual(child->name, BAD_CAST "content") || xmlStrEqual(child->name, BAD_CAST "summary")) {
				xmlChar* content = xmlNodeGetContent(child);
				latest_content = convertHTMLToMarkdown(std::string((char*)content));
				xmlFree(content);
			}
		}
	}

	// Cleanup
	xmlFreeDoc(doc);

	return { latest_title, latest_content, feed_url };
}