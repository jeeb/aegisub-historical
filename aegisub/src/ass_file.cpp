// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file ass_file.cpp
/// @brief Overall storage of subtitle files, undo management and more
/// @ingroup subs_storage

#include "config.h"

#ifndef AGI_PRE
#include <algorithm>
#include <fstream>
#include <list>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#endif

#include "ass_attachment.h"
#include "ass_dialogue.h"
#include "ass_exporter.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "charset_detect.h"
#include "compat.h"
#include "main.h"
#include "subtitle_format.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "utils.h"
#include "version.h"

namespace std {
	template<>
	void swap(AssFile &lft, AssFile &rgt) {
		lft.swap(rgt);
	}
}

/// @brief AssFile constructor
AssFile::AssFile ()
: commitId(-1)
, loaded(false)
{
}

/// @brief AssFile destructor
AssFile::~AssFile() {
	delete_clear(Line);
}

/// @brief Load generic subs
void AssFile::Load (const wxString &_filename,wxString charset,bool addToRecent) {
	bool ok = false;
	Clear();

	try {
		// Try to open file
		FILE *file;
#ifdef WIN32
		file = _tfopen(_filename.c_str(), _T("r"));
#else
		file = fopen(_filename.mb_str(*wxConvFileName), "r");
#endif
		if (!file) {
			throw _T("Unable to open file \"") + _filename + _T("\". Check if it exists and if you have permissions to read it.");
		}
		fclose(file);

		// Find file encoding
		if (charset.empty()) {
			charset = CharSetDetect::GetEncoding(_filename);
		}

		// Generic preparation
		Clear();

		// Get proper format reader
		SubtitleFormat *reader = SubtitleFormat::GetReader(_filename);

		// Read file
		if (reader) {
			AssFile temp;
			reader->SetTarget(&temp);
			reader->ReadFile(_filename,charset);
			swap(temp);
			ok = true;
		}

		// Couldn't find a type
		else throw _T("Unknown file type.");
	}
	catch (agi::UserCancelException const&) { }
	catch (const wchar_t *except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
	}

	catch (wxString except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
	}

	// Real exception
	catch (agi::Exception &e) {
		wxMessageBox(wxString(e.GetChainedMessage().c_str(), wxConvUTF8), L"Error loading file", wxICON_ERROR|wxOK);
	}

	// Other error
	catch (...) {
		wxMessageBox(_T("Unknown error"),_T("Error loading file"),wxICON_ERROR | wxOK);
	}

	// Verify loading
	if (ok) filename = _filename;
	else LoadDefault();

	// Set general data
	loaded = true;

	// Add comments and set vars
	AddComment(_T("Script generated by Aegisub ") + GetAegisubLongVersionString());
	AddComment(_T("http://www.aegisub.org/"));
	SetScriptInfo(_T("ScriptType"),_T("v4.00+"));

	// Push the initial state of the file onto the undo stack
	Commit("", commitId);
	savedCommitId = commitId;

	// Add to recent
	if (addToRecent && ok) AddToRecent(_filename);
}

void AssFile::Save(wxString _filename,bool setfilename,bool addToRecent,const wxString encoding) {
	// Finds last dot
	int i = 0;
	for (i=(int)_filename.size();--i>=0;) {
		if (_filename[i] == '.') break;
	}
	wxString extension = _filename.substr(i+1);
	extension.Lower();

	// Get writer
	SubtitleFormat *writer = SubtitleFormat::GetWriter(_filename);

	// Write file
	if (writer) {
		writer->SetTarget(this);
		writer->WriteFile(_filename,encoding);
	}

	// Couldn't find a type
	else throw _T("Unknown file type.");

	// Add to recent
	if (addToRecent) AddToRecent(_filename);

	// Done
	if (setfilename) {
		savedCommitId = commitId;
		filename = _filename;
	}
}

void AssFile::SaveMemory(std::vector<char> &dst,const wxString encoding) {
	// Set encoding
	wxString enc = encoding;
	if (enc.IsEmpty()) enc = _T("UTF-8");
	if (enc != _T("UTF-8")) throw _T("Memory writer only supports UTF-8 for now.");

	// Check if subs contain at least one style
	// Add a default style if they don't for compatibility with libass/asa
	if (GetStyles().Count() == 0)
		InsertStyle(new AssStyle());

	// Prepare vector
	dst.clear();
	dst.reserve(0x4000);

	// Write file
	entryIter cur;
	unsigned int lineSize = 0;
	unsigned int targetSize = 0;
	unsigned int pos = 0;
	wxCharBuffer buffer;
	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Convert
		wxString temp = (*cur)->GetEntryData() + _T("\r\n");
		buffer = temp.mb_str(wxConvUTF8);
		lineSize = strlen(buffer);

		// Raise capacity if needed
		targetSize = dst.size() + lineSize;
		if (dst.capacity() < targetSize) {
			unsigned int newSize = dst.capacity();
			while (newSize < targetSize) newSize *= 2;
			dst.reserve(newSize);
		}

		// Append line
		pos = dst.size();
		dst.resize(targetSize);
		memcpy(&dst[pos],buffer,lineSize);
	}
}

void AssFile::Export(wxString _filename) {
	AssExporter exporter(this);
	exporter.AddAutoFilters();
	exporter.Export(_filename,_T("UTF-8"));
}

bool AssFile::CanSave() {
	// ASS format?
	wxString ext = filename.Lower().Right(4);
	if (ext == _T(".ass")) return true;

	// Never save texts
	if (ext == _T(".txt")) return false;

	// Check if it's a known extension
	SubtitleFormat *writer = SubtitleFormat::GetWriter(filename);
	if (!writer) return false;

	// Check if format supports timing
	bool canTime = true;
	//if (filename.Lower().Right(4) == _T(".txt")) canTime = false;

	// Scan through the lines
	AssStyle defstyle;
	AssStyle *curstyle;
	AssDialogue *curdiag;
	AssAttachment *attach;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		// Check style, if anything non-default is found, return false
		curstyle = dynamic_cast<AssStyle*>(*cur);
		if (curstyle) {
			if (curstyle->GetEntryData() != defstyle.GetEntryData()) return false;
		}

		// Check for attachments, if any is found, return false
		attach = dynamic_cast<AssAttachment*>(*cur);
		if (attach) return false;

		// Check dialog
		curdiag = dynamic_cast<AssDialogue*>(*cur);
		if (curdiag) {
			// Timed?
			if (!canTime && (curdiag->Start.GetMS() != 0 || curdiag->End.GetMS() != 0)) return false;

			// Overrides?
			curdiag->ParseASSTags();
			for (size_t i=0;i<curdiag->Blocks.size();i++) {
				if (curdiag->Blocks[i]->GetType() != BLOCK_PLAIN) return false;
			}
			curdiag->ClearBlocks();
		}
	}

	// Success
	return true;
}

// I strongly advice you against touching this function unless you know what you're doing;
// even moving things out of order might break ASS parsing - AMZ.
void AssFile::AddLine(wxString data,wxString group,int &version,wxString *outGroup) {
	// Group
	AssEntry *entry = NULL;
	wxString origGroup = group;
	static wxString keepGroup;
	if (!keepGroup.IsEmpty()) group = keepGroup;
	if (outGroup) *outGroup = group;
	wxString lowGroup = group.Lower();

	// Attachment
	if (lowGroup == _T("[fonts]") || lowGroup == _T("[graphics]")) {
		// Check if it's valid data
		size_t dataLen = data.Length();
		bool validData = (dataLen > 0) && (dataLen <= 80);
		for (size_t i=0;i<dataLen;i++) {
			if (data[i] < 33 || data[i] >= 97) validData = false;
		}

		// Is the filename line?
		bool isFilename = (data.StartsWith(_T("fontname: ")) || data.StartsWith(_T("filename: ")));

		// The attachment file is static, since it is built through several calls to this
		// After it's done building, it's reset to NULL
		static AssAttachment *attach = NULL;

		// Attachment exists, and data is over
		if (attach && (!validData || isFilename)) {
			attach->Finish();
			keepGroup.Clear();
			group = origGroup;
			lowGroup = group.Lower();
			Line.push_back(attach);
			attach = NULL;
		}

		// Create attachment if needed
		if (isFilename) {
			attach = new AssAttachment(data.Mid(10));
			attach->group = group;
			keepGroup = group;
			return;
		}

		// Valid data?
		if (attach && validData) {
			// Insert data
			attach->AddData(data);

			// Done building
			if (data.Length() < 80) {
				attach->Finish();
				keepGroup.Clear();
				group = origGroup;
				lowGroup = group.Lower();
				entry = attach;
				attach = NULL;
			}

			// Not done
			else {
				return;
			}
		}
	}

	// Dialogue
	if (lowGroup == _T("[events]")) {
		if (data.StartsWith(_T("Dialogue:")) || data.StartsWith(_T("Comment:"))) {
			AssDialogue *diag = new AssDialogue(data,version);
			//diag->ParseASSTags();
			entry = diag;
			entry->group = group;
		}
		else if (data.StartsWith(_T("Format:"))) {
			entry = new AssEntry(_T("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"));
			entry->group = group;
		}
	}

	// Style
	else if (lowGroup == _T("[v4+ styles]")) {
		if (data.StartsWith(_T("Style:"))) {
			AssStyle *style = new AssStyle(data,version);
			entry = style;
			entry->group = group;
		}
		if (data.StartsWith(_T("Format:"))) {
			entry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
			entry->group = group;
		}
	}

	// Script info
	else if (lowGroup == _T("[script info]")) {
		// Comment
		if (data.StartsWith(_T(";"))) {
			// Skip stupid comments added by other programs
			// Of course, we'll add our own in place later... ;)
			return;
		}

		// Version
		if (data.StartsWith(_T("ScriptType:"))) {
			wxString versionString = data.Mid(11);
			versionString.Trim(true);
			versionString.Trim(false);
			versionString.MakeLower();
			int trueVersion;
			if (versionString == _T("v4.00")) trueVersion = 0;
			else if (versionString == _T("v4.00+")) trueVersion = 1;
			else if (versionString == _T("v4.00++")) trueVersion = 2;
			else throw _T("Unknown SSA file format version");
			if (trueVersion != version) {
				if (!(trueVersion == 2 && version == 1)) wxLogMessage(_T("Warning: File has the wrong extension."));
				version = trueVersion;
			}
		}

		// Everything
		entry = new AssEntry(data);
		entry->group = group;
	}

	// Common entry
	if (entry == NULL) {
		entry = new AssEntry(data);
		entry->group = group;
	}

	// Insert the line
	Line.push_back(entry);
	return;
}

void AssFile::Clear() {
	delete_clear(Line);

	loaded = false;
	filename.clear();
	UndoStack.clear();
	RedoStack.clear();
	undoDescription.clear();
	commitId = -1;
	savedCommitId = 0;
}

void AssFile::LoadDefault (bool defline) {
	Clear();

	// Write headers
	AssStyle defstyle;
	int version = 1;
	AddLine(_T("[Script Info]"),_T("[Script Info]"),version);
	AddLine(_T("Title: Default Aegisub file"),_T("[Script Info]"),version);
	AddLine(_T("ScriptType: v4.00+"),_T("[Script Info]"),version);
	AddLine(_T("WrapStyle: 0"), _T("[Script Info]"),version);
	AddLine(_T("PlayResX: 640"),_T("[Script Info]"),version);
	AddLine(_T("PlayResY: 480"),_T("[Script Info]"),version);
	AddLine(_T("ScaledBorderAndShadow: yes"),_T("[Script Info]"),version);
	AddLine("",_T("[Script Info]"),version);
	AddLine(_T("[V4+ Styles]"),_T("[V4+ Styles]"),version);
	AddLine(_T("Format:  Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"),_T("[V4+ Styles]"),version);
	AddLine(defstyle.GetEntryData(),_T("[V4+ Styles]"),version);
	AddLine("",_T("[V4+ Styles]"),version);
	AddLine(_T("[Events]"),_T("[Events]"),version);
	AddLine(_T("Format:  Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"),_T("[Events]"),version);

	if (defline) {
		AssDialogue def;
		AddLine(def.GetEntryData(),_T("[Events]"),version);
	}

	Commit("");
	savedCommitId = commitId;
	loaded = true;
}

void AssFile::swap(AssFile &that) throw() {
	// Intentionally does not swap undo stack related things
	std::swap(filename, that.filename);
	std::swap(loaded, that.loaded);
	std::swap(commitId, that.commitId);
	std::swap(undoDescription, that.undoDescription);
	std::swap(Line, that.Line);
}

AssFile::AssFile(const AssFile &from)
: undoDescription(from.undoDescription)
, commitId(from.commitId)
, filename(from.filename)
, loaded(from.loaded)
{
	std::transform(from.Line.begin(), from.Line.end(), std::back_inserter(Line), std::mem_fun(&AssEntry::Clone));
}
AssFile& AssFile::operator=(AssFile from) {
	std::swap(*this, from);
	return *this;
}

void AssFile::InsertStyle (AssStyle *style) {
	using std::list;
	AssEntry *curEntry;
	list<AssEntry*>::iterator lastStyle = Line.end();
	list<AssEntry*>::iterator cur;
	wxString lastGroup;

	// Look for insert position
	for (cur=Line.begin();cur!=Line.end();cur++) {
		curEntry = *cur;
		if (curEntry->GetType() == ENTRY_STYLE || (lastGroup == _T("[V4+ Styles]") && curEntry->GetEntryData().substr(0,7) == _T("Format:"))) {
			lastStyle = cur;
		}
		lastGroup = curEntry->group;
	}

	// No styles found, add them
	if (lastStyle == Line.end()) {
		// Add space
		curEntry = new AssEntry("");
		curEntry->group = lastGroup;
		Line.push_back(curEntry);

		// Add header
		curEntry = new AssEntry(_T("[V4+ Styles]"));
		curEntry->group = _T("[V4+ Styles]");
		Line.push_back(curEntry);

		// Add format line
		curEntry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
		curEntry->group = _T("[V4+ Styles]");
		Line.push_back(curEntry);

		// Add style
		style->group = _T("[V4+ Styles]");
		Line.push_back(style);
	}

	// Add to end of list
	else {
		lastStyle++;
		style->group = (*lastStyle)->group;
		Line.insert(lastStyle,style);
	}
}

void AssFile::InsertAttachment (AssAttachment *attach) {
	// Search for insertion point
	std::list<AssEntry*>::iterator insPoint=Line.end(),cur;
	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Check if it's another attachment
		AssAttachment *att = dynamic_cast<AssAttachment*>(*cur);
		if (att) {
			if (attach->group == att->group) insPoint = cur;
		}

		// See if it's the start of group
		else if ((*cur)->GetType() == ENTRY_BASE) {
			AssEntry *entry = (AssEntry*) (*cur);
			if (entry->GetEntryData() == attach->group) insPoint = cur;
		}
	}

	// Found point, insert there
	if (insPoint != Line.end()) {
		insPoint++;
		Line.insert(insPoint,attach);
	}

	// Otherwise, create the [Fonts] group and insert
	else {
		int version=1;
		AddLine("",Line.back()->group,version);
		AddLine(attach->group,attach->group,version);
		Line.push_back(attach);
		AddLine("",attach->group,version);
	}
}

void AssFile::InsertAttachment (wxString filename) {
	wxFileName fname(filename);
	AssAttachment *newAttach = new AssAttachment(fname.GetFullName());

	try {
		newAttach->Import(filename);
	}
	catch (...) {
		delete newAttach;
		throw;
	}

	// Insert
	wxString ext = filename.Right(4).Lower();
	if (ext == _T(".ttf") || ext == _T(".ttc") || ext == _T(".pfb")) newAttach->group = _T("[Fonts]");
	else newAttach->group = _T("[Graphics]");
	InsertAttachment(newAttach);
}

wxString AssFile::GetScriptInfo(const wxString _key) {
	wxString key = _key;;
	key.Lower();
	key += _T(":");
	std::list<AssEntry*>::iterator cur;
	bool GotIn = false;

	for (cur=Line.begin();cur!=Line.end();cur++) {
		if ((*cur)->group == _T("[Script Info]")) {
			GotIn = true;
			wxString curText = (*cur)->GetEntryData();
			curText.Lower();

			if (curText.StartsWith(key)) {
				wxString result = curText.Mid(key.length());
				result.Trim(false);
				result.Trim(true);
				return result;
			}
		}
		else if (GotIn) break;
	}

	return "";
}

int AssFile::GetScriptInfoAsInt(const wxString key) {
	long temp = 0;
	try {
		GetScriptInfo(key).ToLong(&temp);
	}
	catch (...) {
		temp = 0;
	}
	return temp;
}

void AssFile::SetScriptInfo(const wxString _key,const wxString value) {
	wxString key = _key;;
	key.Lower();
	key += _T(":");
	std::list<AssEntry*>::iterator cur;
	std::list<AssEntry*>::iterator prev;
	bool GotIn = false;

	// Look for it
	for (cur=Line.begin();cur!=Line.end();cur++) {
		if ((*cur)->group == _T("[Script Info]")) {
			GotIn = true;
			wxString curText = (*cur)->GetEntryData();
			curText.Lower();

			// Found
			if (curText.StartsWith(key)) {
				// Set value
				if (value != "") {
					wxString result = _key;
					result += _T(": ");
					result += value;
					(*cur)->SetEntryData(result);
				}

				// Remove key
				else {
					Line.erase(cur);
					return;
				}
				return;
			}

			if (!(*cur)->GetEntryData().empty()) prev = cur;
		}

		// Add
		else if (GotIn) {
			if (value != "") {
				wxString result = _key;
				result += _T(": ");
				result += value;
				AssEntry *entry = new AssEntry(result);
				entry->group = (*prev)->group;
				Line.insert(++prev,entry);
			}
			return;
		}
	}
}

void AssFile::GetResolution(int &sw,int &sh) {
	wxString temp = GetScriptInfo(_T("PlayResY"));
	if (temp.IsEmpty() || !temp.IsNumber()) {
		sh = 0;
	}
	else {
		long templ;
		temp.ToLong(&templ);
		sh = templ;
	}

	temp = GetScriptInfo(_T("PlayResX"));
	if (temp.IsEmpty() || !temp.IsNumber()) {
		sw = 0;
	}
	else {
		long templ;
		temp.ToLong(&templ);
		sw = templ;
	}

	// Gabest logic?
	if (sw == 0 && sh == 0) {
		sw = 384;
		sh = 288;
	} else if (sw == 0) {
		if (sh == 1024)
			sw = 1280;
		else
			sw = sh * 4 / 3;
	} else if (sh == 0) {
		// you are not crazy; this doesn't make any sense
		if (sw == 1280)
			sh = 1024;
		else
			sh = sw * 3 / 4;
	}
}

void AssFile::AddComment(const wxString _comment) {
	wxString comment = _T("; ");
	comment += _comment;
	std::list<AssEntry*>::iterator cur;
	int step = 0;

	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Start of group
		if (step == 0 && (*cur)->group == _T("[Script Info]")) step = 1;

		// First line after a ;
		else if (step == 1 && !(*cur)->GetEntryData().StartsWith(_T(";"))) {
			AssEntry *prev = *cur;
			AssEntry *comm = new AssEntry(comment);
			comm->group = prev->group;
			Line.insert(cur,comm);
			break;
		}
	}
}

wxArrayString AssFile::GetStyles() {
	wxArrayString styles;
	AssStyle *curstyle;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		curstyle = dynamic_cast<AssStyle*>(*cur);
		if (curstyle) {
			styles.Add(curstyle->name);
		}
	}
	return styles;
}

AssStyle *AssFile::GetStyle(wxString name) {
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		AssStyle *curstyle = dynamic_cast<AssStyle*>(*cur);
		if (curstyle && curstyle->name == name)
			return curstyle;
	}
	return NULL;
}

void AssFile::AddToRecent(wxString file) {
	config::mru->Add("Subtitle", STD_STR(file));
}

wxString AssFile::GetWildcardList(int mode) {
	if (mode == 0) return SubtitleFormat::GetWildcards(0);
	else if (mode == 1) return _T("Advanced Substation Alpha (*.ass)|*.ass");
	else if (mode == 2) return SubtitleFormat::GetWildcards(1);
	else return "";
}

int AssFile::Commit(wxString desc, int amendId) {
	++commitId;
	// Allow coalescing only if it's the last change and the file has not been
	// saved since the last change and the undo stack isn't empty
	if (commitId == amendId+1 && RedoStack.empty() && savedCommitId != commitId && !UndoStack.empty()) {
		UndoStack.back() = *this;
		return commitId;
	}

	RedoStack.clear();

	// Place copy on stack
	undoDescription = desc;
	UndoStack.push_back(*this);

	// Cap depth
	int depth = OPT_GET("Limits/Undo Levels")->GetInt();
	while ((int)UndoStack.size() > depth) {
		UndoStack.pop_front();
	}

	return commitId;
}

void AssFile::Undo() {
	if (UndoStack.size() <= 1) return;

	RedoStack.push_back(AssFile());
	std::swap(RedoStack.back(), *this);
	UndoStack.pop_back();
	*this = UndoStack.back();
}

void AssFile::Redo() {
	if (RedoStack.empty()) return;

	std::swap(*this, RedoStack.back());
	UndoStack.push_back(*this);
	RedoStack.pop_back();
}

wxString AssFile::GetUndoDescription() const {
	return IsUndoStackEmpty() ? "" : UndoStack.back().undoDescription;
}

wxString AssFile::GetRedoDescription() const {
	return IsRedoStackEmpty() ? "" : RedoStack.back().undoDescription;
}

bool AssFile::CompStart(const AssDialogue* lft, const AssDialogue* rgt) {
	return lft->Start < rgt->Start;
}
bool AssFile::CompEnd(const AssDialogue* lft, const AssDialogue* rgt) {
	return lft->End < rgt->End;
}
bool AssFile::CompStyle(const AssDialogue* lft, const AssDialogue* rgt) {
	return lft->Style < rgt->Style;
}

void AssFile::Sort(CompFunc comp) {
	Sort(Line, comp);
}
namespace {
	struct AssEntryComp : public std::binary_function<const AssEntry*, const AssEntry*, bool> {
		AssFile::CompFunc comp;
		bool operator()(const AssEntry* a, const AssEntry* b) const {
			return comp(static_cast<const AssDialogue*>(a), static_cast<const AssDialogue*>(b));
		}
	};
}
void AssFile::Sort(std::list<AssEntry*> &lst, CompFunc comp) {
	AssEntryComp compE;
	compE.comp = comp;
	// Sort each block of AssDialogues separately, leaving everything else untouched
	for (entryIter begin = lst.begin(); begin != lst.end(); ++begin) {
		if (!dynamic_cast<AssDialogue*>(*begin)) continue;
		entryIter end = begin;
		while (end != lst.end() && dynamic_cast<AssDialogue*>(*end)) ++end;

		// used instead of std::list::sort for partial list sorting
		std::list<AssEntry*> tmp;
		tmp.splice(tmp.begin(), lst, begin, end);
		tmp.sort(compE);
		lst.splice(end, tmp);

		begin = --end;
	}
}
void AssFile::Sort(std::list<AssDialogue*> &lst, CompFunc comp) {
	lst.sort(comp);
}

AssFile *AssFile::top;
