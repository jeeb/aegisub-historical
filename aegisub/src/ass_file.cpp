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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


////////////
// Includes
#include "config.h"

#include <list>
#include <fstream>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include "ass_file.h"
#include "ass_dialogue.h"
#include "ass_style.h"
#include "ass_attachment.h"
#include "ass_override.h"
#include "ass_exporter.h"
#include "vfr.h"
#include "options.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "version.h"
#include "subtitle_format.h"


////////////////////// AssFile //////////////////////
///////////////////////
// AssFile constructor
AssFile::AssFile () {
	AssOverrideTagProto::LoadProtos();
	Clear();
}


//////////////////////
// AssFile destructor
AssFile::~AssFile() {
	Clear();
}


/////////////////////
// Load generic subs
void AssFile::Load (const wxString _filename,const wxString charset,bool addToRecent) {
	bool ok = false;

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
		wxString enc;
		if (charset.IsEmpty()) enc = TextFileReader::GetEncoding(_filename);
		else enc = charset;
		TextFileReader::EnsureValid(enc);

		// Generic preparation
		Clear();

		// Get proper format reader
		SubtitleFormat *reader = SubtitleFormat::GetReader(_filename);

		// Read file
		if (reader) {
			reader->SetTarget(this);
			reader->ReadFile(_filename,enc);
			ok = true;
		}

		// Couldn't find a type
		else throw _T("Unknown file type.");
	}

	// String error
	catch (const wchar_t *except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
	}

	catch (wxString except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
	}

	// Real exception
	catch (Aegisub::Exception &e) {
		wxMessageBox(e.GetChainedMessage(), L"Error loading file", wxICON_ERROR|wxOK);
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

	// Add to recent
	if (addToRecent) AddToRecent(_filename);
}


////////////////////////////
// Save a file to Hard Disk
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
		Modified = false;
		filename = _filename;
	}
}


////////////////////////////////
// Saves a file to a ram vector
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


////////////////////////////////////////////
// Exports file with proper transformations
void AssFile::Export(wxString _filename) {
	AssExporter exporter(this);
	exporter.AddAutoFilters();
	exporter.Export(_filename,_T("UTF-8"));
}


//////////////////
// Can save file?
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
		curstyle = AssEntry::GetAsStyle(*cur);
		if (curstyle) {
			if (curstyle->GetEntryData() != defstyle.GetEntryData()) return false;
		}

		// Check for attachments, if any is found, return false
		attach = AssEntry::GetAsAttachment(*cur);
		if (attach) return false;

		// Check dialog
		curdiag = AssEntry::GetAsDialogue(*cur);
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


////////////////////////////////////
// Returns script as a single string
//wxString AssFile::GetString() {
//	using std::list;
//	wxString ret;
//	AssEntry *entry;
//	ret += 0xfeff;
//	for (list<AssEntry*>::iterator cur=Line.begin();cur!=Line.end();) {
//		entry = *cur;
//		ret += entry->GetEntryData();
//		ret += L"\n";
//		cur++;
//	}
//	return ret;
//}


///////////////////////
// Appends line to Ass
// -------------------
// I strongly advice you against touching this function unless you know what you're doing;
// even moving things out of order might break ASS parsing - AMZ.
//
int AssFile::AddLine (wxString data,wxString group,int lasttime,int &version,wxString *outGroup) {
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
		bool isFilename = (data.Left(10) == _T("fontname: ") || data.Left(10) == _T("filename: "));

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
			attach->SetStartMS(lasttime);
			attach->group = group;
			keepGroup = group;
			return lasttime;
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
				return lasttime;
			}
		}
	}

	// Dialogue
	if (lowGroup == _T("[events]")) {
		if ((data.Left(9) == _T("Dialogue:") || data.Left(8) == _T("Comment:"))) {
			AssDialogue *diag = new AssDialogue(data,version);
			lasttime = diag->GetStartMS();
			//diag->ParseASSTags();
			entry = diag;
			entry->SetStartMS(lasttime);
			entry->group = group;
		}
		if (data.Left(7) == _T("Format:")) {
			entry = new AssEntry(_T("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"));
			entry->SetStartMS(lasttime);
			entry->group = group;
		}
	}

	// Style
	else if (lowGroup == _T("[v4+ styles]")) {
		if (data.Left(6) == _T("Style:")) {
			AssStyle *style = new AssStyle(data,version);
			entry = style;
			entry->SetStartMS(lasttime);
			entry->group = group;
		}
		if (data.Left(7) == _T("Format:")) {
			entry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
			entry->SetStartMS(lasttime);
			entry->group = group;
		}
	}

	// Script info
	else if (lowGroup == _T("[script info]")) {
		// Comment
		if (data.Left(1) == _T(";")) {
			// Skip stupid comments added by other programs
			// Of course, we'll add our own in place later... ;)
			return lasttime;
		}

		// Version
		if (data.Left(11) == _T("ScriptType:")) {
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
		entry->SetStartMS(lasttime);
		entry->group = group;
	}

	// Common entry
	if (entry == NULL) {
		entry = new AssEntry(data);
		entry->SetStartMS(lasttime);
		entry->group = group;
	}

	// Insert the line
	Line.push_back(entry);
	return lasttime;
}


//////////////////////////////
// Clears contents of assfile
void AssFile::Clear () {
	for (std::list<AssEntry*>::iterator cur=Line.begin();cur != Line.end();cur++) {
		if (*cur) delete *cur;
	}
	Line.clear();

	loaded = false;
	filename = _T("");
	Modified = false;
}


//////////////////////
// Loads default subs
void AssFile::LoadDefault (bool defline) {
	// Clear first
	Clear();

	// Write headers
	AssStyle defstyle;
	int version = 1;
	AddLine(_T("[Script Info]"),_T("[Script Info]"),-1,version);
	AddLine(_T("Title: Default Aegisub file"),_T("[Script Info]"),-1,version);
	AddLine(_T("ScriptType: v4.00+"),_T("[Script Info]"),-1,version);
	AddLine(_T("WrapStyle: 0"), _T("[Script Info]"),-1,version);
	AddLine(_T("PlayResX: 640"),_T("[Script Info]"),-1,version);
	AddLine(_T("PlayResY: 480"),_T("[Script Info]"),-1,version);
	AddLine(_T("ScaledBorderAndShadow: yes"),_T("[Script Info]"),-1,version);
	AddLine(_T(""),_T("[Script Info]"),-1,version);
	AddLine(_T("[V4+ Styles]"),_T("[V4+ Styles]"),-1,version);
	AddLine(_T("Format:  Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"),_T("[V4+ Styles]"),-1,version);
	AddLine(defstyle.GetEntryData(),_T("[V4+ Styles]"),-1,version);
	AddLine(_T(""),_T("[V4+ Styles]"),-1,version);
	AddLine(_T("[Events]"),_T("[Events]"),-1,version);
	AddLine(_T("Format:  Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"),_T("[Events]"),-1,version);

	if (defline) {
		AssDialogue def;
		AddLine(def.GetEntryData(),_T("[Events]"),0,version);
	}

	loaded = true;
}


////////////////////
// Copy constructor
AssFile::AssFile (AssFile &from) {
	using std::list;

	// Copy standard variables
	filename = from.filename;
	loaded = from.loaded;
	Modified = from.Modified;

	// Copy lines
	for (list<AssEntry*>::iterator cur=from.Line.begin();cur!=from.Line.end();cur++) {
		Line.push_back((*cur)->Clone());
	}
}


//////////////////////
// Insert a new style
void AssFile::InsertStyle (AssStyle *style) {
	// Variables
	using std::list;
	AssEntry *curEntry;
	list<AssEntry*>::iterator lastStyle = Line.end();
	list<AssEntry*>::iterator cur;
	int lasttime = -1;
	wxString lastGroup;

	// Look for insert position
	for (cur=Line.begin();cur!=Line.end();cur++) {
		curEntry = *cur;
		if (curEntry->GetType() == ENTRY_STYLE || (lastGroup == _T("[V4+ Styles]") && curEntry->GetEntryData().substr(0,7) == _T("Format:"))) {
			lastStyle = cur;
		}
		lasttime = curEntry->GetStartMS();
		lastGroup = curEntry->group;
	}

	// No styles found, add them
	if (lastStyle == Line.end()) {
		// Add space
		curEntry = new AssEntry(_T(""));
		curEntry->group = lastGroup;
		curEntry->SetStartMS(lasttime);
		Line.push_back(curEntry);

		// Add header
		curEntry = new AssEntry(_T("[V4+ Styles]"));
		curEntry->group = _T("[V4+ Styles]");
		curEntry->SetStartMS(lasttime);
		Line.push_back(curEntry);

		// Add format line
		curEntry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
		curEntry->group = _T("[V4+ Styles]");
		curEntry->SetStartMS(lasttime);
		Line.push_back(curEntry);

		// Add style
		style->group = _T("[V4+ Styles]");
		style->SetStartMS(lasttime);
		Line.push_back(style);
	}

	// Add to end of list
	else {
		lastStyle++;
		style->group = (*lastStyle)->group;
		style->SetStartMS(lasttime);
		Line.insert(lastStyle,style);
	}
}


////////////////////
// Insert attachment
void AssFile::InsertAttachment (AssAttachment *attach) {
	// Search for insertion point
	std::list<AssEntry*>::iterator insPoint=Line.end(),cur;
	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Check if it's another attachment
		AssAttachment *att = AssEntry::GetAsAttachment(*cur);
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
		attach->SetStartMS((*insPoint)->GetStartMS());
		Line.insert(insPoint,attach);
	}

	// Otherwise, create the [Fonts] group and insert
	else {
		int version=1;
		int StartMS = Line.back()->GetStartMS();
		AddLine(_T(""),Line.back()->group,StartMS,version);
		AddLine(attach->group,attach->group,StartMS,version);
		attach->SetStartMS(StartMS);
		Line.push_back(attach);
		AddLine(_T(""),attach->group,StartMS,version);
	}
}


///////////////////////////////
// Insert attachment from file
void AssFile::InsertAttachment (wxString filename) {
	// Get name
	wxFileName fname(filename);

	// Create
	AssAttachment *newAttach = new AssAttachment(fname.GetFullName());

	// Load
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


////////////////////
// Gets script info
wxString AssFile::GetScriptInfo(const wxString _key) {
	// Prepare
	wxString key = _key;;
	key.Lower();
	key += _T(":");
	std::list<AssEntry*>::iterator cur;
	bool GotIn = false;

	// Look for it
	for (cur=Line.begin();cur!=Line.end();cur++) {
		if ((*cur)->group == _T("[Script Info]")) {
			GotIn = true;
			wxString curText = (*cur)->GetEntryData();
			curText.Lower();

			// Found
			if (curText.Left(key.length()) == key) {
				wxString result = curText.Mid(key.length());
				result.Trim(false);
				result.Trim(true);
				return result;
			}
		}
		else if (GotIn) break;
	}

	// Couldn't find
	return _T("");
}


//////////////////////////
// Get script info as int
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


//////////////////////////
// Set a script info line
void AssFile::SetScriptInfo(const wxString _key,const wxString value) {
	// Prepare
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
			if (curText.Left(key.length()) == key) {
				// Set value
				if (value != _T("")) {
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
			if (value != _T("")) {
				wxString result = _key;
				result += _T(": ");
				result += value;
				AssEntry *entry = new AssEntry(result);
				entry->group = (*prev)->group;
				entry->SetStartMS((*prev)->GetStartMS());
				Line.insert(++prev,entry);
			}
			return;
		}
	}
}


//////////////////
// Get resolution
void AssFile::GetResolution(int &sw,int &sh) {
	// Height
	wxString temp = GetScriptInfo(_T("PlayResY"));
	if (temp.IsEmpty() || !temp.IsNumber()) {
		sh = 0;
	}
	else {
		long templ;
		temp.ToLong(&templ);
		sh = templ;
	}

	// Width
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
		if (sw == 1280)
			sh = 1024;
		else
			sh = sw * 3 / 4;
	}
}


///////////////////////////////////
// Adds a comment to [Script Info]
void AssFile::AddComment(const wxString _comment) {
	wxString comment = _T("; ");
	comment += _comment;
	std::list<AssEntry*>::iterator cur;
	int step = 0;

	// Find insert position
	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Start of group
		if (step == 0 && (*cur)->group == _T("[Script Info]")) step = 1;

		// First line after a ;
		else if (step == 1 && (*cur)->GetEntryData().Left(1) != _T(";")) {
			AssEntry *prev = *cur;
			AssEntry *comm = new AssEntry(comment);
			comm->group = prev->group;
			comm->SetStartMS(prev->GetStartMS());
			Line.insert(cur,comm);
			break;
		}
	}
}


//////////////////////
// Get list of styles
wxArrayString AssFile::GetStyles() {
	wxArrayString styles;
	AssStyle *curstyle;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		curstyle = AssEntry::GetAsStyle(*cur);
		if (curstyle) {
			styles.Add(curstyle->name);
		}
	}
	return styles;
}


///////////////////////////////
// Gets style of specific name
AssStyle *AssFile::GetStyle(wxString name) {
	AssStyle *curstyle;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		curstyle = AssEntry::GetAsStyle(*cur);
		if (curstyle) {
			if (curstyle->name == name) return curstyle;
		}
	}
	return NULL;
}


////////////////////////////////////
// Adds file name to list of recent
void AssFile::AddToRecent(wxString file) {
	Options.AddToRecentList(file,_T("Recent sub"));
}


///////////////////////////////
// List of supported wildcards
wxString AssFile::GetWildcardList(int mode) {
	//if (mode == 0) return _T("All Supported Types (*.ass,*.ssa,*.srt,*.txt,*.mkv,*.mks,*.mka)|*.ass;*.ssa;*.srt;*.txt;*.mkv;*.mks;*.mka|Advanced Substation Alpha (*.ass)|*.ass|Substation Alpha (*.ssa)|*.ssa|SubRip (*.srt)|*.srt|Plain-text (*.txt)|*.txt|Matroska (*.mkv,*.mks,*.mka)|*.mkv;*.mks;*.mka");
	//else if (mode == 1) return _T("Advanced Substation Alpha (*.ass)|*.ass");
	//else if (mode == 2) return _T("All Supported Types (*.ass,*.ssa,*.srt,*.txt,*.mkv,*.mks,*.mka)|*.ass;*.ssa;*.srt;*.txt|Advanced Substation Alpha (*.ass)|*.ass|Substation Alpha (*.ssa)|*.ssa|SubRip (*.srt)|*.srt|Plain-text (*.txt)|*.txt");
	//else return _T("");

	if (mode == 0) return SubtitleFormat::GetWildcards(0);
	else if (mode == 1) return _T("Advanced Substation Alpha (*.ass)|*.ass");
	else if (mode == 2) return SubtitleFormat::GetWildcards(1);
	else return _T("");
}


////////////////////////////////////////////
// Compress/decompress for storage on stack
void AssFile::CompressForStack(bool compress) {
	AssDialogue *diag;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		diag = AssEntry::GetAsDialogue(*cur);
		if (diag) {
			if (compress) {
				diag->SetEntryData(_T(""));
				diag->ClearBlocks();
			}
			else diag->UpdateData();
		}
	}
}


//////////////////////////////
// Checks if file is modified
bool AssFile::IsModified() {
	return Modified;
}


/////////////////////////
// Flag file as modified
void AssFile::FlagAsModified(wxString desc) {
	// Clear redo
	if (!RedoStack.empty()) {
		//StackPush();
		//UndoStack.push_back(new AssFile(*UndoStack.back()));
		for (std::list<AssFile*>::iterator cur=RedoStack.begin();cur!=RedoStack.end();cur++) {
			delete *cur;
		}
		RedoStack.clear();
	}

	Modified = true;
	StackPush(desc);
}


//////////////
// Stack push
void AssFile::StackPush(wxString desc) {
	// Places copy on stack
	AssFile *curcopy = new AssFile(*top);
	curcopy->CompressForStack(true);
	curcopy->undodescription = desc;
	UndoStack.push_back(curcopy);
	StackModified = true;

	// Cap depth
	int n = 0;
	for (std::list<AssFile*>::iterator cur=UndoStack.begin();cur!=UndoStack.end();cur++) {
		n++;
	}
	int depth = Options.AsInt(_T("Undo levels"));
	while (n > depth) {
		delete UndoStack.front();
		UndoStack.pop_front();
		n--;
	}
}


/////////////
// Stack pop
void AssFile::StackPop() {
	bool addcopy = false;
	wxString undodesc=_T("");
	

	if (StackModified) {
		undodesc=UndoStack.back()->undodescription;
		delete UndoStack.back();
		UndoStack.pop_back();
		StackModified = false;
		addcopy = true;
	}

	if (!UndoStack.empty()) {
		//delete top;
		AssFile *undo = UndoStack.back();
		top->CompressForStack(true);
		top->undodescription = undodesc;
		RedoStack.push_back(top);
		top = undo;
		top->CompressForStack(false);
		UndoStack.pop_back();
		Popping = true;
	}

	if (addcopy) {
		StackPush(top->undodescription);
	}
}


//////////////
// Stack redo
void AssFile::StackRedo() {

	bool addcopy = false;
	if (StackModified) {
		delete UndoStack.back();
		UndoStack.pop_back();
		StackModified = false;
		addcopy = true;
	}

	if (!RedoStack.empty()) {
		top->CompressForStack(true);
		UndoStack.push_back(top);
		top = RedoStack.back();
		top->CompressForStack(false);
		RedoStack.pop_back();
		Popping = true;
		//StackModified = false;
	}

	if (addcopy) {
		StackPush(top->undodescription);
	}
}


///////////////
// Stack clear
void AssFile::StackClear() {
	// Clear undo
	for (std::list<AssFile*>::iterator cur=UndoStack.begin();cur!=UndoStack.end();cur++) {
		delete *cur;
	}
	UndoStack.clear();

	// Clear redo
	for (std::list<AssFile*>::iterator cur=RedoStack.begin();cur!=RedoStack.end();cur++) {
		delete *cur;
	}
	RedoStack.clear();

	Popping = false;
}


///////////////
// Stack reset
void AssFile::StackReset() {
	StackClear();
	delete top;
	top = new AssFile;
	StackModified = false;
}


//////////////////////////////////
// Returns if undo stack is empty
bool AssFile::IsUndoStackEmpty() {
	if (StackModified) return (UndoStack.size() <= 1);
	else return UndoStack.empty();
}


//////////////////////////////////
// Returns if redo stack is empty
bool AssFile::IsRedoStackEmpty() {
	return RedoStack.empty();
}

wxString AssFile::GetUndoDescription() {
	return (IsUndoStackEmpty())?_T(""):(UndoStack.back())->undodescription;
}

wxString AssFile::GetRedoDescription() {
	return (IsRedoStackEmpty())?_T(""):(RedoStack.back())->undodescription;

}

//////////
// Global
AssFile *AssFile::top;
std::list<AssFile*> AssFile::UndoStack;
std::list<AssFile*> AssFile::RedoStack;
bool AssFile::Popping;
bool AssFile::StackModified;




