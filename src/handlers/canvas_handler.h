#pragma once

#include <dpp/dpp.h>
#include "../config/config.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <json/json.h>

struct AssignmentInfo {
	std::string id;
	std::string name;
	std::string grading_type;
	bool graded;
	std::string graded_at;
};

class CanvasHandler {
public:
	CanvasHandler(dpp::cluster& bot, CanvasConfig& config, const std::string& api_token);
	void start();

private:
	dpp::cluster& bot_;
	CanvasConfig& config_;
	std::string api_token_;

	std::unordered_map<std::string, AssignmentInfo> assignments_;
	std::unordered_map<std::string, AssignmentInfo> ungraded_assignments_;

	bool isNullOrWhitespace(const Json::Value& value) const;
	void checkAssignments();
	void checkSubmissions();
	std::vector<AssignmentInfo> fetchAssignments();
	int fetchSubmissionsForAssignment(const std::string& assignment_id, const std::string& assignment_name);

	void log(const std::string& message);
};