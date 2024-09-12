#include <std_include.hpp>

#include "io.hpp"
#include "properties.hpp"


namespace utils::properties
{
	namespace
	{
		std::string get_properties_file()
		{
			return "h2m-update-config.json";
		}

		rapidjson::Document load_properties()
		{
			rapidjson::Document default_doc{};
			default_doc.SetObject();

			std::string data{};
			const auto& props = get_properties_file();
			if (!io::read_file(props, &data))
			{
				return default_doc;
			}

			rapidjson::Document doc{};
			const rapidjson::ParseResult result = doc.Parse(data);

			if (!result || !doc.IsObject())
			{
				return default_doc;
			}

			return doc;
		}

		void store_properties(const rapidjson::Document& doc)
		{
			rapidjson::StringBuffer buffer{};
			rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::Document::EncodingType, rapidjson::ASCII<>>
				writer(buffer);
			doc.Accept(writer);

			const std::string json{ buffer.GetString(), buffer.GetLength() };

			const auto& props = get_properties_file();
			io::write_file(props, json);
		}
	}

	std::optional<bool> load(const std::string& name)
	{
		const auto doc = load_properties();

		if (!doc.HasMember(name))
		{
			return {};
		}

		const auto& value = doc[name];
		if (!value.IsBool())
		{
			return {};
		}

		return { value.GetBool() };
	}

	void store(const std::string& name, const bool& value)
	{
		auto doc = load_properties();

		while (doc.HasMember(name))
		{
			doc.RemoveMember(name);
		}

		rapidjson::Value key{};
		key.SetString(name, doc.GetAllocator());

		rapidjson::Value member{};
		member.SetBool(value);

		doc.AddMember(key, member, doc.GetAllocator());

		store_properties(doc);
	}
}
