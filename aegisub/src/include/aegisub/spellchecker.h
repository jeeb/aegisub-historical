// Copyright (c) 2006, Rodrigo Braz Monteiro
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

/// @file spellchecker.h
/// @brief Declaration of base-class for spell checkers
/// @ingroup main_headers spelling
///

#pragma once

#ifndef AGI_PRE
#include <wx/arrstr.h>
#include <wx/string.h>
#endif

#include "factory_manager.h"

/// @class SpellChecker
/// @brief DOCME
///
/// DOCME
class SpellChecker {
public:
	/// @brief DOCME
	///
	virtual ~SpellChecker() {}

	/// @brief DOCME
	/// @param word 
	/// @return 
	///
	virtual void AddWord(wxString word) {}

	/// @brief DOCME
	/// @param word 
	/// @return 
	///
	virtual bool CanAddWord(wxString word) { return false; }

	virtual bool CheckWord(wxString word)=0;
	virtual wxArrayString GetSuggestions(wxString word)=0;

	virtual wxArrayString GetLanguageList()=0;
	virtual void SetLanguage(wxString language)=0;
};

/// DOCME
/// @class SpellCheckerFactoryManager
/// @brief DOCME
///
/// DOCME
class SpellCheckerFactory : public Factory0<SpellChecker> {
public:
	static SpellChecker *GetSpellChecker();
	static void RegisterProviders();
};
