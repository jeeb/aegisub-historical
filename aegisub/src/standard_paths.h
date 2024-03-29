// Copyright (c) 2007, Rodrigo Braz Monteiro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file standard_paths.h
/// @see standard_paths.cpp
/// @ingroup utility
///




///////////
// Headers
#ifndef AGI_PRE
#include <map>
#endif


/// DOCME
/// @class StandardPaths
/// @brief DOCME
///
/// DOCME
class StandardPaths {
private:
	static StandardPaths &GetInstance();


	/// DOCME
	std::map<wxString,wxString> paths;

	StandardPaths();
	StandardPaths(StandardPaths const&);
	StandardPaths& operator=(StandardPaths const&);

	wxString DoDecodePath(wxString path);
	wxString DoEncodePath(const wxString &path);
	void DoSetPathValue(const wxString &path, const wxString &value);

public:

	/// @brief DOCME
	/// @param path 
	/// @return 
	///
	static wxString DecodePath(const wxString &path) { return GetInstance().DoDecodePath(path); }
	static wxString DecodePathMaybeRelative(const wxString &path, const wxString &relativeTo);

	/// @brief DOCME
	/// @param path 
	/// @return 
	///
	static wxString EncodePath(const wxString &path) { return GetInstance().DoEncodePath(path); }

	/// @brief DOCME
	/// @param path  
	/// @param value 
	///
	static void SetPathValue(const wxString &path, const wxString &value) { GetInstance().DoSetPathValue(path,value); }
};


