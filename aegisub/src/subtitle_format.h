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

/// @file subtitle_format.h
/// @see subtitle_format.cpp
/// @ingroup subtitle_io
///

#pragma once

#ifndef AGI_PRE
#include <list>

#include <wx/arrstr.h>
#include <wx/string.h>
#endif

#include <libaegisub/exception.h>

class AssFile;
class AssEntry;

/// DOCME
/// @class SubtitleFormat
/// @brief DOCME
///
/// DOCME
class SubtitleFormat {
	/// DOCME
	bool isCopy;

	/// DOCME
	AssFile *assFile;

	void Register();
	void Remove();

	/// DOCME
	static std::list<SubtitleFormat*> formats;

	/// DOCME
	static bool loaded;

protected:
	/// DOCME
	struct FPSRational {

		/// DOCME
		int num;

		/// DOCME
		int den;

		/// DOCME
		bool smpte_dropframe;
	};

	/// DOCME
	std::list<AssEntry*> *Line;

	void CreateCopy();
	void ClearCopy();
	void SortLines();
	void ConvertTags(int format,const wxString &lineEnd,bool mergeLineBreaks=true);
	//void Merge(bool identical,bool overlaps,bool stripComments,bool stripNonDialogue);
	void StripComments();
	void StripNonDialogue();
	void RecombineOverlaps();
	void MergeIdentical();

	void Clear();
	void LoadDefault(bool defline=true);

	/// @brief DOCME
	/// @return 
	///
	AssFile *GetAssFile() { return assFile; }
	void AddLine(wxString data,wxString group,int &version,wxString *outgroup=NULL);
	FPSRational AskForFPS(bool showSMPTE=false);

	virtual wxString GetName()=0;
	virtual wxArrayString GetReadWildcards();
	virtual wxArrayString GetWriteWildcards();

public:
	SubtitleFormat();
	virtual ~SubtitleFormat();
	void SetTarget(AssFile *file);

	static wxString GetWildcards(int mode);

	/// @brief DOCME
	/// @param filename 
	/// @return 
	///
	virtual bool CanReadFile(wxString filename) { return false; };

	/// @brief DOCME
	/// @param filename 
	/// @return 
	///
	virtual bool CanWriteFile(wxString filename) { return false; };

	/// @brief DOCME
	/// @param filename      
	/// @param forceEncoding 
	///
	virtual void ReadFile(wxString filename,wxString forceEncoding=_T("")) { };

	/// @brief DOCME
	/// @param filename 
	/// @param encoding 
	///
	virtual void WriteFile(wxString filename,wxString encoding=_T("")) { };

	static SubtitleFormat *GetReader(wxString filename);
	static SubtitleFormat *GetWriter(wxString filename);
	static void LoadFormats();
	static void DestroyFormats();
};

DEFINE_SIMPLE_EXCEPTION(SubtitleFormatParseError, agi::InvalidInputException, "subtitle_io/parse/generic")
