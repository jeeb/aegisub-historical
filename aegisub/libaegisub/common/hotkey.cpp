// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// $Id$

/// @file hotkey.cpp
/// @brief Hotkey handler
/// @ingroup hotkey menu event window

#include "../config.h"

#ifndef LAGI_PRE
#include <cmath>
#include <memory>
#endif

#ifdef _WIN32
#include <tuple>
#else
#include <tr1/tuple>
#endif

#include "libaegisub/hotkey.h"

#include "libaegisub/access.h"
#include "libaegisub/cajun/writer.h"
#include "libaegisub/exception.h"
#include "libaegisub/io.h"
#include "libaegisub/json.h"
#include "libaegisub/log.h"

namespace agi {
	namespace hotkey {

Hotkey *hotkey;

std::string Combo::Str() const {
	std::string str(key_map[0]);
	str.reserve(str.size() + (key_map.size() - 1) * 2);
	for (unsigned int i=1; i < key_map.size(); i++) {
		str.append("-" + key_map[i]);
	}
	return str;
}

std::string Combo::StrMenu() const {
	return Str();
}

void Hotkey::ComboInsert(Combo const& combo) {
	str_map.insert(std::make_pair(combo.Str(), combo));
	cmd_map.insert(std::make_pair(combo.CmdName(), combo));
}

Hotkey::~Hotkey() {
	Flush();
}

Hotkey::Hotkey(const std::string &file, const std::string &default_config)
: config_file(file)
{
	LOG_D("hotkey/init") << "Generating hotkeys.";

	json::UnknownElement hotkey_root = agi::json_util::file(config_file, default_config);
	json::Object object = hotkey_root;

	for (json::Object::const_iterator index(object.Begin()); index != object.End(); index++) {
		const json::Object::Member& member = *index;
		const json::Object& obj = member.element;
		BuildHotkey(member.name, obj);
	}
}


void Hotkey::BuildHotkey(std::string const& context, const json::Object& object) {
	for (json::Object::const_iterator index(object.Begin()); index != object.End(); index++) {
		const json::Object::Member& member = *index;
		const json::Array& array = member.element;

		for (json::Array::const_iterator arr_index(array.Begin()); arr_index != array.End(); arr_index++) {
			Combo combo(context, member.name);

			const json::Object& obj = *arr_index;

			const json::Array& arr_mod = obj["modifiers"];

			if (arr_mod.Size() > 0) {
				for (json::Array::const_iterator arr_mod_index(arr_mod.Begin()); arr_mod_index != arr_mod.End(); arr_mod_index++) {
					const json::String& key_mod = *arr_mod_index;
					combo.KeyInsert(key_mod.Value());
				} // for arr_mod_index
			}
			combo.KeyInsert(static_cast<const json::String&>(obj["key"]).Value());
			ComboInsert(combo);
		} // for arr_index
	} // for index
}

bool Hotkey::Scan(const std::string &context, const std::string &str, bool always, std::string &cmd) const {
	std::string local, dfault;

	HotkeyMap::const_iterator index, end;
	for (std::tr1::tie(index, end) = str_map.equal_range(str); index != end; ++index) {
		std::string const& ctext = index->second.Context();

		if (always && ctext == "Always") {
			cmd = index->second.CmdName();
			LOG_D("agi/hotkey/found") << "Found: " << str << "  Context (req/found): " << context << "/Always   Command: " << cmd;
			return true;
		}
		if (ctext == "Default") {
			dfault = index->second.CmdName();
		}
		else if (ctext == context) {
			local = index->second.CmdName();
		}
	}

	if (!local.empty()) {
		cmd = local;
		LOG_D("agi/hotkey/found") << "Found: " << str << "  Context: " << context << "  Command: " << local;
		return true;
	}
	if (!dfault.empty()) {
		cmd = dfault;
		LOG_D("agi/hotkey/found") << "Found: " << str << "  Context (req/found): " << context << "/Default   Command: " << dfault;
		return true;
	}

	return false;
}

std::vector<std::string> Hotkey::GetHotkeys(const std::string &context, const std::string &command) const {
	std::vector<std::string> ret;

	HotkeyMap::const_iterator it, end;
	for (std::tr1::tie(it, end) = cmd_map.equal_range(command); it != end; ++it) {
		std::string ctext = it->second.Context();
		if (ctext == "Always" || ctext == "Default" || ctext == context) {
			ret.push_back(it->second.StrMenu());
		}
	}

	sort(ret.begin(), ret.end());
	ret.erase(unique(ret.begin(), ret.end()), ret.end());

	return ret;
}

std::string Hotkey::GetHotkey(const std::string &context, const std::string &command) const {
	std::string ret;
	HotkeyMap::const_iterator it, end;
	for (std::tr1::tie(it, end) = cmd_map.equal_range(command); it != end; ++it) {
		std::string ctext = it->second.Context();
		if (ctext == context) return it->second.StrMenu();
		if (ctext == "Default") ret = it->second.StrMenu();
		else if (ret.empty() && ctext == "Always") it->second.StrMenu();
	}
	return ret;
}

void Hotkey::Flush() {
	json::Object root;

	for (HotkeyMap::iterator index = str_map.begin(); index != str_map.end(); ++index) {
		Combo::ComboMap combo_map(index->second.Get());

		json::Array modifiers;
		for (int i = 0; i != combo_map.size()-1; i++) {
			modifiers.Insert(json::String(combo_map[i]));
		}

		json::Object hotkey;
		hotkey["modifiers"] = modifiers;
		hotkey["key"] = json::String(combo_map.back());

		json::Object& context_obj = root[index->second.Context()];
		json::Array& combo_array = context_obj[index->second.CmdName()];

		combo_array.Insert(hotkey);
	}

	io::Save file(config_file);
	json::Writer::Write(root, file.Get());
}

	} // namespace toolbar
} // namespace agi
