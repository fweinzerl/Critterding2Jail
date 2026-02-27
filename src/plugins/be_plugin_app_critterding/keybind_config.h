#ifndef CD_KEYBIND_CONFIG_H_INCLUDED
#define CD_KEYBIND_CONFIG_H_INCLUDED

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct CdKeybindConfig
{
	bool forward_horizontal_only = false;
	bool allow_roll_for_movement = true;
	float turbo_multiplier = 3.0f;
	std::unordered_map<std::string, std::vector<std::string>> action_keys;
};

inline std::string cd_to_lower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
	return value;
}

inline std::string cd_read_file(const std::string& path)
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
	{
		return std::string();
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

inline std::string cd_extract_object(const std::string& source, const std::string& key)
{
	const auto key_pos = source.find("\"" + key + "\"");
	if (key_pos == std::string::npos)
	{
		return std::string();
	}
	const auto object_start = source.find('{', key_pos);
	if (object_start == std::string::npos)
	{
		return std::string();
	}

	int depth = 0;
	for (size_t i = object_start; i < source.size(); ++i)
	{
		if (source[i] == '{')
		{
			++depth;
		}
		else if (source[i] == '}')
		{
			--depth;
			if (depth == 0)
			{
				return source.substr(object_start, i - object_start + 1);
			}
		}
	}
	return std::string();
}

inline std::vector<std::string> cd_extract_keys_array(const std::string& array_text)
{
	std::vector<std::string> keys;
	std::regex key_re("\"([^\"]+)\"");
	auto begin = std::sregex_iterator(array_text.begin(), array_text.end(), key_re);
	auto end = std::sregex_iterator();
	for (auto it = begin; it != end; ++it)
	{
		keys.push_back(cd_to_lower((*it)[1].str()));
	}
	return keys;
}

inline std::vector<std::string> cd_expand_key_alias(const std::string& key)
{
	if (key == "shift")
	{
		return { "left shift", "right shift" };
	}
	if (key == "ctrl" || key == "control")
	{
		return { "left control", "right control" };
	}
	if (key == "alt")
	{
		return { "left alt", "right alt" };
	}
	return { key };
}

inline void cd_parse_action_keys(CdKeybindConfig& config, const std::string& actions_object)
{
	std::regex action_re("\"([^\"]+)\"\\s*:\\s*\\{[\\s\\S]*?\"default_keys\"\\s*:\\s*\\[([\\s\\S]*?)\\]");
	auto begin = std::sregex_iterator(actions_object.begin(), actions_object.end(), action_re);
	auto end = std::sregex_iterator();
	for (auto it = begin; it != end; ++it)
	{
		const auto action_name = (*it)[1].str();
		const auto keys_raw = cd_extract_keys_array((*it)[2].str());
		std::vector<std::string> expanded;
		for (const auto& key : keys_raw)
		{
			const auto aliases = cd_expand_key_alias(key);
			expanded.insert(expanded.end(), aliases.begin(), aliases.end());
		}
		config.action_keys[action_name] = expanded;
	}
}

inline void cd_parse_movement(CdKeybindConfig& config, const std::string& movement_object)
{
	std::regex forward_mode_re("\"forward_mode\"\\s*:\\s*\"([^\"]+)\"");
	std::smatch match;
	if (std::regex_search(movement_object, match, forward_mode_re))
	{
		config.forward_horizontal_only = (cd_to_lower(match[1].str()) == "horizontal_only");
	}

	std::regex allow_roll_re("\"allow_roll_for_movement\"\\s*:\\s*(true|false)");
	if (std::regex_search(movement_object, match, allow_roll_re))
	{
		config.allow_roll_for_movement = (match[1].str() == "true");
	}

	std::regex turbo_re("\"turbo_multiplier\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)");
	if (std::regex_search(movement_object, match, turbo_re))
	{
		config.turbo_multiplier = std::stof(match[1].str());
	}
}

inline CdKeybindConfig load_cd_keybind_config()
{
	CdKeybindConfig config;
	auto load_one = [&config](const std::string& path)
	{
		auto text = cd_read_file(path);
		if (text.empty())
		{
			return;
		}

		const auto actions_object = cd_extract_object(text, "actions");
		if (!actions_object.empty())
		{
			cd_parse_action_keys(config, actions_object);
		}

		const auto movement_object = cd_extract_object(text, "movement");
		if (!movement_object.empty())
		{
			cd_parse_movement(config, movement_object);
		}
	};

	// Default baseline.
	load_one("config/keybinds.default.json");
	load_one("../config/keybinds.default.json");
	// Optional local override.
	load_one("config/keybinds.local.json");
	load_one("../config/keybinds.local.json");

	return config;
}

inline std::vector<std::string> get_cd_action_keys(
	const CdKeybindConfig& config,
	const std::string& action,
	const std::vector<std::string>& fallback)
{
	auto found = config.action_keys.find(action);
	if (found == config.action_keys.end() || found->second.empty())
	{
		std::vector<std::string> expanded_fallback;
		for (const auto& key : fallback)
		{
			const auto aliases = cd_expand_key_alias(cd_to_lower(key));
			expanded_fallback.insert(expanded_fallback.end(), aliases.begin(), aliases.end());
		}
		return expanded_fallback;
	}
	return found->second;
}

#endif
