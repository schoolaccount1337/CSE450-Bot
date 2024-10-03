#include "canvas_handler.h"
#include <curl/curl.h>
#include <json/json.h>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <string>

constexpr int CURL_REQUEST_DELAY = 15;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
	size_t newLength = size * nmemb;
	try {
		s->append(static_cast<char*>(contents), newLength);
		return newLength;
	}
	catch (const std::bad_alloc& e) {
		return 0;
	}
}

CanvasHandler::CanvasHandler(dpp::cluster& bot, CanvasConfig& config, const std::string& api_token)
	: bot_(bot), config_(config), api_token_(api_token) {
	log("CanvasHandler initialized with course_id: " + config_.course_id);
}

void CanvasHandler::start() {
	std::thread([this]() {
		while (true) {
			try {
				log("Reloading config file");
				config_ = Config::getInstance().getCanvasConfig();

				log("Starting assignment check...");
				checkAssignments();

				log("Starting submission check...");
				checkSubmissions();

				std::this_thread::sleep_for(std::chrono::seconds(config_.check_interval));
			}
			catch (const std::exception& e) {
				log("Exception in main loop: " + std::string(e.what()));
			}
		}
		}).detach();
}

void CanvasHandler::checkAssignments() {
	log("Fetching assignments...");
	std::vector<AssignmentInfo> fetched_assignments;
	try {
		std::this_thread::sleep_for(std::chrono::seconds(CURL_REQUEST_DELAY)); // Using the constant delay
		fetched_assignments = fetchAssignments();
	}
	catch (const std::exception& e) {
		log("Exception in checkAssignments: " + std::string(e.what()));
		return;
	}

	log("Processing fetched assignments...");
	for (const auto& assignment : fetched_assignments) {
		if (assignment.id.empty() || assignment.name.empty() || assignment.grading_type == "not_graded") {
			log("Skipping assignment due to empty ID, name, or non-gradable type.");
			continue;
		}

		// Check if the assignment already exists in the assignments_
		if (assignments_.find(assignment.id) == assignments_.end()) {
			// New assignment
			std::string message_content = "<@&" + config_.ping_role_id + "> A new assignment \"" + assignment.name + "\" was added.";
			dpp::message msg(config_.discord_channel_id, message_content);
			msg.allowed_mentions.parse_roles = true;
			bot_.message_create(msg);

			log("New assignment: " + assignment.name);

			// Add to assignments_ and ungraded_assignments_
			assignments_[assignment.id] = assignment;
			ungraded_assignments_[assignment.id] = assignment;

			log("Assignment is added to ungraded: " + assignment.name);
		}
		else {
			log("Assignment \"" + assignment.name + "\" already exists and won't be added again.");
		}
	}

	log("Current ungraded assignments:");
	for (const auto& [id, assignment] : ungraded_assignments_) {
		log(" - " + assignment.name);
	}
}

std::vector<AssignmentInfo> CanvasHandler::fetchAssignments() {
	CURL* curl;
	CURLcode res;
	std::string response_string;
	std::vector<AssignmentInfo> all_assignments;
	int page = 1;
	bool has_more_pages = true;
	const int per_page = 20;

	while (has_more_pages) {
		curl = curl_easy_init();
		if (!curl) {
			log("Failed to initialize curl.");
			return {};
		}

		std::string url = config_.api_url + "courses/" + config_.course_id +
			"/assignments?page=" + std::to_string(page) +
			"&per_page=" + std::to_string(per_page);

		log("Fetching URL: " + url);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

		struct curl_slist* headers = nullptr;
		std::string auth_header = "Authorization: Bearer " + api_token_;
		headers = curl_slist_append(headers, auth_header.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		log("Performing curl request for page: " + std::to_string(page));
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			log("Failed to fetch assignments (page " + std::to_string(page) + "): " + std::string(curl_easy_strerror(res)));
			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);
			return all_assignments;
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		log("Response size for page " + std::to_string(page) + ": " + std::to_string(response_string.size()) + " bytes");

		Json::Value root;
		Json::CharReaderBuilder readerBuilder;
		std::string errs;

		try {
			log("Parsing JSON response for page " + std::to_string(page));
			std::istringstream response_stream(response_string);
			if (!Json::parseFromStream(readerBuilder, response_stream, &root, &errs)) {
				throw std::runtime_error("Failed to parse assignments JSON: " + errs);
			}
		}
		catch (const std::exception& e) {
			log("Exception in JSON parsing (page " + std::to_string(page) + "): " + std::string(e.what()));
			return all_assignments;
		}

		log("Parsed JSON successfully for page " + std::to_string(page));
		for (const auto& assignment_json : root) {
			AssignmentInfo assignment;
			assignment.id = assignment_json["id"].asString();
			assignment.name = assignment_json["name"].asString();
			assignment.grading_type = assignment_json["grading_type"].asString();

			// Only add gradable assignments
			if (assignment.grading_type != "not_graded") {
				assignment.graded = false;
				assignment.graded_at.clear();
				all_assignments.push_back(assignment);
				log("Fetched gradable assignment: " + assignment.name);
			}
			else {
				log("Skipping non-gradable assignment: " + assignment.name);
			}
		}

		if (root.size() < per_page) {
			has_more_pages = false;
		}
		else {
			page++;
		}

		response_string.clear();
	}

	return all_assignments;
}

void CanvasHandler::checkSubmissions() {
	log("Starting submission check...");

	log("Printing what is currently in ungraded_assignments_");
	for (auto it = ungraded_assignments_.begin(); it != ungraded_assignments_.end(); ) {
		log(it->second.name);
		++it;
	}
	log("DONE.");

	int ret = 0;

	for (auto it = ungraded_assignments_.begin(); it != ungraded_assignments_.end(); ) {
		if (it->first.empty() || it->second.name.empty()) {
			log("Skipping submission check due to empty assignment ID or name.");
			++it;
			continue;
		}

		ret = 0;
		try {
			log("Fetching submissions for assignment: " + it->second.name);
			std::this_thread::sleep_for(std::chrono::seconds(CURL_REQUEST_DELAY));  // Delay to avoid API rate limits
			ret = fetchSubmissionsForAssignment(it->first, it->second.name);
		}
		catch (const std::exception& e) {
			log("Exception in checkSubmissions: " + std::string(e.what()));
			++it;
			continue;
		}

		if (ret == 1) {
			log("Now removing \"" + it->second.name + "\"from list of ungraded assignments");
			it = ungraded_assignments_.erase(it);
		}
		else {
			log("Assignment \"" + it->second.name + "\" is still in list of ungraded assignments.");
			++it;
		}
	}
}

int CanvasHandler::fetchSubmissionsForAssignment(const std::string& assignment_id, const std::string& assignment_name) {
	log("Fetching submissions for assignment: \"" + assignment_name + "\"");

	if (assignment_id.empty() || assignment_name.empty()) {
		log("Skipping submission check due to empty assignment ID or name.");
		return -1;
	}

	CURL* curl;
	CURLcode res;
	std::string response_string;

	curl = curl_easy_init();
	if (!curl) {
		log("Failed to initialize curl.");
		return -1;
	}

	std::string url = config_.api_url + "courses/" + config_.course_id + "/assignments/" + assignment_id + "/submissions/self";
	log("Fetching submission URL: " + url);

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

	struct curl_slist* headers = nullptr;
	std::string auth_header = "Authorization: Bearer " + api_token_;
	headers = curl_slist_append(headers, auth_header.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		log("Failed to fetch submission: " + std::string(curl_easy_strerror(res)));
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		return -1;
	}

	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);

	log("Response size for submission: " + std::to_string(response_string.size()) + " bytes");

	Json::Value root;
	Json::CharReaderBuilder readerBuilder;
	std::string errs;

	try {
		log("Parsing submission JSON...");
		std::istringstream response_stream(response_string);
		if (!Json::parseFromStream(readerBuilder, response_stream, &root, &errs)) {
			throw std::runtime_error("Failed to parse submission JSON: " + errs);
		}
	}
	catch (const std::exception& e) {
		log("Exception in JSON parsing (submissions): " + std::string(e.what()));
		return -1;
	}

	if (!isNullOrWhitespace(root["graded_at"])) {
		std::string graded_at = root["graded_at"].asString();
		log("Submission graded_at value: \"" + graded_at + "\"");

		std::string message_content = "<@&" + config_.ping_role_id + "> Grades for \"" + assignment_name + "\" have been released.";
		dpp::message msg(config_.discord_channel_id, message_content);
		msg.allowed_mentions.parse_roles = true;
		bot_.message_create(msg);

		log("Grades released for assignment: " + assignment_name);

		return 1;
	}
	log("Assignment \"" + assignment_name + "\" is still ungraded.");
	return 0;
}

void CanvasHandler::log(const std::string& message) {
	bot_.log(dpp::ll_info, message);
}

bool CanvasHandler::isNullOrWhitespace(const Json::Value& value) const {
	if (value.isNull()) {
		return true;
	}

	std::string str = value.asString();
	return str.empty() || std::all_of(str.begin(), str.end(), isspace);
}