// Copyright (c) 2009, Amar Takhar <verm@aegisub.org>
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

/// @file platform_unix_bsd.h
/// @see platform_unix_bsd.cpp
/// @ingroup unix

class Platform;

/// @brief BSD values.
class PlatformUnixOSX : public PlatformUnix {
public:
	PlatformUnixOSX() {};
	virtual ~PlatformUnixOSX() {};

	// Hardware
	virtual std::string CPUId();
	virtual std::string CPUSpeed();
	virtual int CPUCores();
	virtual int CPUCount();
	virtual std::string CPUFeatures();
	virtual std::string CPUFeatures2();
	virtual uint64_t Memory();

	virtual std::string PatchLevel();
	virtual std::string QuickTimeExt();
	virtual std::string HardwareModel();

	// Unix Specific
	virtual std::string UnixLibraries();
};
