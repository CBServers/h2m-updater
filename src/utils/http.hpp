#pragma once

#include <string>
#include <optional>
#include <future>

namespace utils::http
{
	using headers = std::unordered_map<std::string, std::string>;

	std::optional<std::string> get_data(const std::string& url, const headers& headers = {}, const std::function<void(size_t, size_t)>& callback = {}, uint32_t retries = 2);
	bool get_data_stream(const std::string& url, const headers& headers = {}, const std::function<void(size_t, size_t)>& progress_callback_ = {}, const std::function<void(const char*, size_t)>& stream_callback = {}, uint32_t retries = 2);
	std::future<std::optional<std::string>> get_data_async(const std::string& url, const headers& headers = {});
}