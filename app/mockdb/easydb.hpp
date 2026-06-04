#pragma once

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "namedkey.h"
#include "../type/deviceinfo.h"
#include "../../third/json.hpp"

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DeviceName, deviceName, manufacturer, software)

class EasyDB
{
	public:
	EasyDB()
	{
		filename = "./db.json";
		load();
	}

	template <typename T>
	bool read(T &t)
	{
		std::string key = NamedKey<T>()();
		auto it = m_datas.find(key);
		if (it == m_datas.end()) {
			return false;
		}
		json j = json::parse(it->second, nullptr, false);
		if (j.is_discarded()) {
			return false;
		}
		j.get_to(t);
		return true;
	}

	template <typename T>
	void write(const T &t)
	{
		std::string key = NamedKey<T>()();
		m_datas[key] = json(t).dump();

		dump();
	}

	void load()
	{
		std::ifstream in_file(filename);
		if (!in_file.is_open()) {
			return;
		}
		std::string content = std::string(
				std::istreambuf_iterator<char>(in_file),
				std::istreambuf_iterator<char>()
				);
		in_file.close();

		json j = json::parse(content, nullptr, false);
		if (j.is_discarded() || !j.is_object()) {
			return;
		}
		for (auto& [k, v] : j.items()) {
			if (v.is_string()) {
				m_datas[k] = v.get<std::string>();
			}
		}
	}

	void dump()
	{
		json j = json::object();
		for (const auto& [k, v] : m_datas) {
			j[k] = v;
		}

		std::ofstream out_file(filename);
		if (out_file.is_open()) {
			out_file << j.dump();
			out_file.close();
		}
	}
	private:
		std::map<std::string,std::string> m_datas;
		std::string filename;
};

NAMEDKEY(DeviceName)
