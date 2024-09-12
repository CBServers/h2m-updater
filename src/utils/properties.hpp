#pragma once

#include <filesystem>
#include <optional>

namespace utils::properties
{
	std::optional<bool> load(const std::string& name);
	void store(const std::string& name, const bool& value);
}
