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

/// @file option.h
/// @brief Public interface for option values.
/// @ingroup libaegisub

#pragma once

#ifndef LAGI_PRE
#include <fstream>
#include <map>
#endif

#include <libaegisub/exception.h>

namespace json {
	class Object;
	class UnknownElement;
}

namespace agi {

DEFINE_BASE_EXCEPTION_NOINNER(OptionError,Exception)
DEFINE_SIMPLE_EXCEPTION_NOINNER(OptionErrorNotFound, OptionError, "options/not_found")
DEFINE_SIMPLE_EXCEPTION_NOINNER(OptionErrorDuplicateKey, OptionError, "options/dump/duplicate")

class OptionValue;

/// This is a cool trick: make a class un-copyable, in this case we always want
/// to update our *original* map, this will ensure that it is always updated in
/// every situation.
class OptionValueMap : public std::map<std::string,OptionValue*> {
private:
	OptionValueMap(const OptionValueMap& x);
	OptionValueMap& operator=(const OptionValueMap& x);
public:
	OptionValueMap() {};
};

class Options {
	friend class PutOptionVisitor;

public:
	/// Options class settings.
	enum OptionSetting {
		NONE       = 0x000,		///< Do nothing (default)
		FLUSH_SKIP = 0x001,		///< Skip writing the config file to disk
	};

private:
	/// Internal OptionValueMap
	OptionValueMap values;

	/// @brief Create option object.
	/// @param path Path to store
	::json::Object CreateObject(std::string path);

	/// User config (file that will be written to disk)
	const std::string config_file;

	/// Default config (for use when config file is/gets corrupted)
	const std::string config_default;

	/// Whether the user (final) config has been loaded
	bool config_loaded;

	/// Settings.
	const OptionSetting setting;

	/// @brief Load a config file into the Options object.
	/// @param config Config to load.
	void LoadConfig(std::istream& stream);


protected:
	/// @brief Write an option to file.
	/// @param[out] obj  Parent object
	/// @param[in] path  Path option should be stored in.
	/// @param[in] value Value to write.
	static bool PutOption(::json::Object &obj, const std::string &path, const ::json::UnknownElement &value);


public:
	/// @brief Constructor
	/// @param file User config that will be loaded from and written back to.
	/// @param default_config Default configuration.
	Options(const std::string &file, const std::string &default_config, const OptionSetting setting=NONE);

	/// Destructor
	~Options();

	/// @brief Get an option by name.
	/// @param name Option to get.
	/// Get an option value object by name throw an internal exception if the option is not found.
	OptionValue* Get(const std::string &name);

	/// @brief Next configuration file to load.
	/// @param[in] src Stream to load from.
	/// Load next config which will superceed any values from previous configs
	/// can be called as many times as required, but only after ConfigDefault() and
	/// before ConfigUser()
	void ConfigNext(std::istream &stream);

	/// @brief Set user config file.
	/// Set the user configuration file and read options from it, closes all possible
	/// config file loading and sets the file to write to.
	void ConfigUser();

	/// Write the user configuration to disk, throws an exeption if something goes wrong.
	void Flush();

	/// Print internal option type, name and values.
	void DumpAll();
};

} // namespace agi
