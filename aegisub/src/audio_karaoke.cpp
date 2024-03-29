// Copyright (c) 2005-2009, Rodrigo Braz Monteiro, Niels Martin Hansen
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

/// @file audio_karaoke.cpp
/// @brief Karaoke table UI in audio box (not in audio display)
/// @ingroup audio_ui
///

#include "config.h"

#ifndef AGI_PRE
#include <algorithm>

#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/tokenzr.h>
#endif

#include <libaegisub/log.h>

#include "ass_override.h"
#include "audio_box.h"
#include "audio_controller.h"
#include "audio_display.h"
#include "audio_karaoke.h"
#include "selection_controller.h"

/// @brief Empty constructor 
///
AudioKaraokeSyllable::AudioKaraokeSyllable()
: AssKaraokeSyllable()
, start_time(0), selected(false)
, display_w(0), display_x(0)
{
}

/// @brief Copy-from-base constructor 
/// @param base 
///
AudioKaraokeSyllable::AudioKaraokeSyllable(const AssKaraokeSyllable &base)
: AssKaraokeSyllable(base)
, start_time(0), selected(false)
, display_w(0), display_x(0)
{
}

/// @brief Constructor 
/// @param parent 
///
AudioKaraoke::AudioKaraoke(wxWindow *parent)
: wxWindow (parent,-1,wxDefaultPosition,wxSize(10,5),wxTAB_TRAVERSAL|wxBORDER_SUNKEN)
{
	LOG_D("karaoke/audio") << "Constructor";
	enabled = false;
	splitting = false;
	split_cursor_syl = -1;
	curSyllable = 0;
	diag = 0;
	workDiag = 0;
}

/// @brief Destructor 
///
AudioKaraoke::~AudioKaraoke() {
	delete workDiag;
}

/// @brief Load from dialogue 
/// @param _diag 
/// @return 
///
bool AudioKaraoke::LoadFromDialogue(AssDialogue *_diag) {
	LOG_D("karaoke/audio") << "diag=" << _diag;
	// Make sure we're not in splitting-mode
	if (splitting) {
		LOG_D("karaoke/audio") << "is splitting, discarding splits";
		// Discard any splits and leave split-mode
		EndSplit(false);
	}

	// Set dialogue
	delete workDiag;
	diag = _diag;
	if (!diag) {
		LOG_D("karaoke/audio") << "no diag, refreshing and returning flase";
		Refresh(false);
		return false;
	}

	// Split
	LOG_D("karaoke/audio") << "split";
	workDiag = new AssDialogue(diag->GetEntryData(), false);
	workDiag->ParseASSTags();
	must_rebuild = false;
	bool hasKar = ParseDialogue(workDiag);

	// No karaoke, autosplit
	if (!hasKar) {
		LOG_D("karaoke/audio") << "no existing karaoke, auto-splitting";
		AutoSplit();
	}

	// Done
	//if (curSyllable < 0) curSyllable = syllables.size()-1;
	//if (curSyllable >= (signed) syllables.size()) curSyllable = 0;
	//SetSelection(curSyllable);
	//Refresh(false);
	LOG_D("karaoke/audio") << "returning " << hasKar;
	return !hasKar;
}

/// @brief Writes line back 
/// @return 
///
void AudioKaraoke::Commit() {
	LOG_D("karaoke/audio") << "commit";
	if (splitting) {
		LOG_D("karaoke/audio") << "splitting, ending split";
		EndSplit(true);
	}
	wxString finalText = _T("");
	AudioKaraokeSyllable *syl;
	size_t n = syllables.size();
	LOG_D("karaoke/audio") << "syllabels.size() = " << n;
	if (must_rebuild) {
		LOG_D("karaoke/audio") << "must_rebuild";
		workDiag->ClearBlocks();
		for (size_t i=0;i<n;i++) {
			syl = &syllables.at(i);
			finalText += wxString::Format(_T("{%s%i}"), syl->type.c_str(), syl->duration) + syl->text;
		}
		workDiag->Text = finalText;
		workDiag->ParseASSTags();
		diag->Text = finalText;
	} else {
		LOG_D("karaoke/audio") << "updating karaoke without rebuild";
		for (size_t i = 0; i < n; i++) {
			LOG_D("karaoke/audio") << "updating syllabel: " << i;
			syl = &syllables.at(i);
			LOG_D("karaoke/audio") << "syllabel pointer: " << syl << "; tagdata pointer: " << syl->tag << "; length: " << syl->duration;
			// Some weird people have text before the first karaoke tag on a line.
			// Check that a karaoke tag actually exists for the (non-)syllable to avoid a crash.
			if (syl->tag && syl->tag->Params.size()>0)
				syl->tag->Params[0]->Set<int>(syl->duration);
			// Of course, if the user changed the duration of such a non-syllable, its timing can't be updated and will stay zero.
			// There is no way to check for that right now, and I can't bother to fix it.
		}
		LOG_D("karaoke/audio") << "done updating syllabels";
		workDiag->UpdateText();
		workDiag->ClearBlocks();
		workDiag->ParseASSTags();
		diag->Text = workDiag->Text;
	}
	LOG_D("karaoke/audio") << "returning";
}

/// @brief Autosplit line 
/// @return 
///
void AudioKaraoke::AutoSplit() {
	LOG_D("karaoke/audio") << "autosplit";

	// Get lengths
	int timeLen = (diag->End.GetMS() - diag->Start.GetMS())/10;
	int letterlen = diag->Text.Length();
	int round = letterlen / 2;
	int curlen;
	int acumLen = 0;
	wxString newText;

	// Parse words
	wxStringTokenizer tkz(diag->Text,_T(" "),wxTOKEN_RET_DELIMS);
	wxArrayString words;
	while (tkz.HasMoreTokens()) {
		words.Add(tkz.GetNextToken());
	}

	// Process words
	for (size_t i=0;i<words.Count();i++) {
		curlen = (words[i].Length() * timeLen + round) / letterlen;
		acumLen += curlen;

		// Don't let it accumulate over total length
		if (acumLen > timeLen) {
			curlen -= acumLen - timeLen;
			acumLen = timeLen;
		}

		// Ensure that it accumulates all of it
		if (i == words.Count()-1 && acumLen < timeLen) {
			curlen += timeLen - acumLen;
			acumLen = timeLen;
		}

		newText += wxString::Format(_T("{\\k%i}"),curlen) + words[i];
	}

	// Workaround for bug #503
	// Make the line one blank syllable if it's completely blank
	if (newText == _T("")) newText = wxString::Format(_T("{\\k%d}"), timeLen);

	// Load
	must_rebuild = true;
	AssDialogue newDiag(diag->GetEntryData());
	newDiag.Text = newText;
	newDiag.ParseASSTags();
	ParseDialogue(&newDiag);

	LOG_D("karaoke/audio") << "returning";
}

/// @brief Parses text to extract karaoke 
/// @param curDiag 
/// @return 
///
bool AudioKaraoke::ParseDialogue(AssDialogue *curDiag) {
	// parse the tagdata
	AssKaraokeVector tempsyls;
	ParseAssKaraokeTags(curDiag, tempsyls);

	bool found_kara = tempsyls.size() > 1;

	// copy base syllables to real
	syllables.clear();
	syllables.reserve(tempsyls.size());
	int cur_time = 0;
	for (AssKaraokeVector::iterator base = tempsyls.begin(); base != tempsyls.end(); ++base) {
		AudioKaraokeSyllable fullsyl(*base);
		fullsyl.start_time = cur_time;
		cur_time += fullsyl.duration;
		syllables.push_back(fullsyl);
	}

	// if first syllable is empty, remove it
	if (!syllables[0].unstripped_text) {
		syllables.erase(syllables.begin());
		found_kara = syllables.size() > 0;
	}

	// if there's more than one syllable in the list, at least one karaoke tag was found
	return found_kara;
}

/// @brief Set syllable 
/// @param n 
/// @return 
///
void AudioKaraoke::SetSyllable(int n) {
	LOG_D("karaoke/audio") << "n=" << n;
	if (n == -1) n = syllables.size()-1;
	if (n >= (signed) syllables.size()) n = 0;
	curSyllable = n;
	startClickSyl = n;
	SetSelection(n);
	Refresh(false);
	LOG_D("karaoke/audio") << "returning";
}

///////////////
// Event table
BEGIN_EVENT_TABLE(AudioKaraoke,wxWindow)
	EVT_PAINT(AudioKaraoke::OnPaint)
	EVT_SIZE(AudioKaraoke::OnSize)
	EVT_MOUSE_EVENTS(AudioKaraoke::OnMouse)
END_EVENT_TABLE()

/// @brief Paint event 
/// @param event 
///
void AudioKaraoke::OnPaint(wxPaintEvent &event) {
	// Get dimensions
	int w,h;
	GetClientSize(&w,&h);

	// Start Paint
	wxPaintDC dc(this);

	// Draw background
	dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0,0,w,h);

	// Set syllable font
	wxFont curFont(9,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,_T("Verdana"),wxFONTENCODING_SYSTEM); // FIXME, hardcoded font
	dc.SetFont(curFont);

	// Draw syllables
	if (enabled) {
		wxString temptext;
		size_t syln = syllables.size();
		int dx = 0;
		int tw,th;
		int delta;
		int dlen;
		for (size_t i=0;i<syln;i++) {
			AudioKaraokeSyllable &syl = syllables.at(i);
			// Calculate text length
			temptext = syl.text;
			// If we're splitting, every character must be drawn
			if (!splitting) {
				temptext.Trim(true);
				temptext.Trim(false);
			}
			GetTextExtent(temptext,&tw,&th,NULL,NULL,&curFont);
			delta = 0;
			if (tw < 10) delta = 10 - tw;
			dlen = tw + 8 + delta;

			// Draw border
			//dc.SetPen(wxPen(wxColour(0,0,0)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			if (syl.selected && !splitting) {
				dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
				dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
			}
			else {
				dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)));
				dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
			}
			dc.DrawRectangle(dx,0,dlen,h);

			// Put syllable-end-marker after text
			dc.SetPen(*wxBLACK_PEN);
			//dc.DrawLine(dx, 0, dx, h);
			dc.DrawLine(dx, 0, dx+1, 0);
			dc.DrawLine(dx, h-1, dx+1, h-1);
			dc.DrawLine(dx+dlen-1, 0, dx+dlen-1, h);
			dc.DrawLine(dx+dlen-2, 0, dx+dlen-1, 0);
			dc.DrawLine(dx+dlen-2, h-1, dx+dlen-1, h-1);

			// Draw text
			if (splitting) {
				// Make sure the text position is more predictable in case of splitting
				dc.DrawText(temptext,dx+4,(h-th)/2);
			} else {
				dc.DrawText(temptext,dx+(delta/2)+4,(h-th)/2);
			}

			// Draw pending splits
			if (syl.pending_splits.size() > 0) {
				wxArrayInt widths;
				if (dc.GetPartialTextExtents(temptext, widths)) {
					for (unsigned int j = 0; j < syl.pending_splits.size(); j++) {
						dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
						int splitxpos = dx + 4;
						// Handle splitters placed before first char in syllable; these are represented as -1
						if (syl.pending_splits[j] >= 0) {
							splitxpos += widths[syl.pending_splits[j]];
						} else {
							splitxpos += 0;
						}
						dc.DrawLine(splitxpos, 0, splitxpos, h);
					}
				} else {
					wxLogError(_T("Karaoke syllable display: Failed to GetPartialTextExtents. This should never happen, except on severely overloaded systems."));
				}
			}

			if (splitting && split_cursor_syl == (signed)i /*&& split_cursor_x > 0*/) {
				dc.SetPen(*wxRED);
				dc.DrawLine(dx+4+split_cursor_x, 0, dx+4+split_cursor_x, h);
				dc.SetPen(wxPen(wxColour(0,0,0)));
			}

			// Set syllable data
			syl.display_x = dx;
			syl.display_w = dlen;

			// Increment dx
			dx += dlen;
		}
	}

	event.Skip();
}

/// @brief Size event 
/// @param event 
///
void AudioKaraoke::OnSize(wxSizeEvent &event) {
	Refresh(false);
}

/// @brief Mouse event 
/// @param event 
/// @return 
///
void AudioKaraoke::OnMouse(wxMouseEvent &event) {
	// Get coordinates
	int x = event.GetX();
	//int y = event.GetY();
	bool shift = event.m_shiftDown;

	// Syllable selection mode
	if (!splitting) {
		int syl = GetSylAtX(x);

		// Button pressed
		if (event.LeftDown() || event.RightDown()) {
			if (syl != -1) {
				if (shift) {
					SetSelection(syl,startClickSyl);
					Refresh(false);
				}

				else {
					SetSelection(syl);
					startClickSyl = syl;
					curSyllable = syl;
					Refresh(false);
					display->Update();
					CaptureMouse();
				}
			}
		}
		// Dragging to make a selection
		else if (event.Dragging() && (event.LeftIsDown() || event.RightIsDown())) {
			if (syl < 0) syl = 0;
			SetSelection(syl, startClickSyl);
			Refresh(false);
		}
		// Released left button
		else if (event.LeftUp() && HasCapture()) {
			ReleaseMouse();
		}
		// Released right button; make a menu for selecting \k type
		else if (event.RightUp()) {
			if (HasCapture())
				ReleaseMouse();

			AudioKaraokeTagMenu menu(this);
			PopupMenu(&menu);
		}
	}
	
	// Karaoke syllable splitting mode
	else {
		int syli = GetSylAtX(x);

		// Valid syllable
		if (syli != -1) {
			AudioKaraokeSyllable &syl = syllables.at(syli);

			// Get the widths after each character in the text
			wxClientDC dc(this);
			wxFont curFont(9,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,_T("Verdana"),wxFONTENCODING_SYSTEM);
			dc.SetFont(curFont);
			wxArrayInt widths;
			dc.GetPartialTextExtents(syl.text, widths);

			// Find the character closest to the mouse
			int rx = x - syl.display_x - 4;
			int split_cursor_char = -2;
			split_cursor_syl = -1;
			split_cursor_x = -1;
			if (syl.text.Len() > 0) {
				int lastx = 0;
				split_cursor_syl = syli;
				for (unsigned int i = 0; i < widths.size(); i++) {
					//wx Log Debug(_T("rx=%d, lastx=%d, widths[i]=%d, i=%d, widths.size()=%d, syli=%d"), rx, lastx, widths[i], i, widths.size(), syli);
					if (lastx - rx < widths[i] - rx) {
						if (rx - lastx < widths[i] - rx) {
							//wx Log Debug(_T("Found at PREV!"));
							split_cursor_x = lastx;
							split_cursor_char = i - 1;
							break;
						} else if (rx < widths[i]) {
							//wx Log Debug(_T("Found at CURRENT!"));
							split_cursor_x = widths[i];
							split_cursor_char = i;
							break;
						}
					}
					lastx = widths[i];
				}
				// If no split-point was caught by the for-loop, it must be over the last character,
				// ie. split at next-to-last position
				if (split_cursor_x < 0) {
					//wx Log Debug(_T("Emergency picking LAST!"));
					split_cursor_x = widths[widths.size()-1];
					split_cursor_char = widths.size() - 1;
				}
			}

			// Do something if there was a click and we're at a valid position
			if (event.LeftDown() && split_cursor_char >= -1) {
				//wx Log Debug(_T("A click!"));
				int num_removed = 0;
				std::vector<int>::iterator i = syl.pending_splits.begin();
				while (i != syl.pending_splits.end()) {
					if (split_cursor_char == *i) {
						//wx Log Debug(_T("Erasing entry"));
						num_removed++;
						syl.pending_splits.erase(i);
						break;
					} else {
						i++;
					}
				}
				if (num_removed == 0) {
					syl.pending_splits.push_back(split_cursor_char);
				}
			}

		}
		
		// Invalid syllable (int syli = GetSylAtX(x); returned -1)
		else {
			split_cursor_syl = -1;
			split_cursor_x = -1;
		}

		if (split_cursor_syl >= 0) {
			Refresh(false);
		}
	}
}

/// @brief Get Syllable at position X 
/// @param x 
/// @return 
///
int AudioKaraoke::GetSylAtX(int x) {
	int dx,dw;
	size_t syln = syllables.size();
	for (size_t i=0;i<syln;i++) {
		dx = syllables.at(i).display_x; 
		dw = syllables.at(i).display_w; 
		if (x >= dx && x < dx+dw) {
			return i;
		}
	}
	return -1;
}

/// @brief Set selection 
/// @param start 
/// @param end   
///
void AudioKaraoke::SetSelection(int start,int end) {
	LOG_D("karaoke/audio") << "start=" << start << " end=" << end;
	// Default end
	if (end == -1) end = start;

	// Get min/max range
	size_t min = start;
	size_t max = end;
	if (max < min) {
		size_t temp = max;
		max = min;
		min = temp;
	}
	LOG_D("karaoke/audio") << "min=" << min << ", max=" << max;

	// Set values
	bool state;
	size_t syls = syllables.size();
	int sels = 0;
	for (size_t i=0;i<syls;i++) {
		state = (i >= min && i <= max);
		syllables.at(i).selected = state;
		if (state) sels++;
	}
	curSyllable = min;
	selectionCount = max-min+1;
	LOG_D("karaoke/audio") << "new curSyllabel=" << curSyllable << ", selectionCount=" << selectionCount;

	// Set box buttons
	box->SetKaraokeButtons();
}

/// @brief Join syllables 
/// @return 
///
void AudioKaraoke::Join() {
	LOG_D("karaoke/audio") << "join";
	// Variables
	bool gotOne = false;
	size_t syls = syllables.size();
	AudioKaraokeSyllable *curSyl;
	int first = 0;

	// Loop
	for (size_t i=0;i<syls;i++) {
		curSyl = &syllables.at(i);
		LOG_D("karaoke/audio") << "syllabel " << i << ", text='" << curSyl->text.c_str() << "', unstripped_text='" << curSyl->unstripped_text.c_str() << "', duration=" << curSyl->duration;
		if (curSyl->selected) {
			if (!gotOne) {
				gotOne = true;
				first = i;
			}
			else {
				AudioKaraokeSyllable &work = syllables.at(first);
				LOG_D("karaoke/audio") << "worksyl " << work.text.c_str() << ", text='" << work.text.c_str() << ", unstripped_text='" << work.unstripped_text.c_str() << "', duration=" << work.duration;
				work.duration += curSyl->duration;
				work.text += curSyl->text;
				work.unstripped_text += curSyl->unstripped_text;
				syllables.erase(syllables.begin()+i);
				i--;
				syls--;
			}
		}
	}

	// Set selection
	SetSelection(first);

	// Update
	must_rebuild = true;
	//display->NeedCommit = true;
	display->Update();
	Refresh(false);

	LOG_D("karaoke/audio") << "returning";
}

/// @brief Enter splitting-mode 
///
void AudioKaraoke::BeginSplit() {
	LOG_D("karaoke/audio") << "beginsplit";
	splitting = true;
	split_cursor_syl = -1;
	split_cursor_x = -1;
	box->SetKaraokeButtons();
	Refresh(false);
}

/// @brief Leave splitting-mode, committing changes 
/// @param commit 
/// @return 
///
void AudioKaraoke::EndSplit(bool commit) {
	LOG_D("karaoke/audio") << "endsplit, commit=" << commit;
	splitting = false;
	bool hasSplit = false;
	size_t first_sel = ~0U;
	for (size_t i = 0; i < syllables.size(); i ++) {
		if (syllables[i].pending_splits.size() > 0) {
			if (commit) {
				if (syllables[i].selected && i < first_sel)
					first_sel = i;
				SplitSyl(i);
				hasSplit = true;
			} else {
				syllables[i].pending_splits.clear();
			}
		}
	}

	// Update
	if (hasSplit) {
		LOG_D("karaoke/audio") << "hassplit";
		must_rebuild = true;
		//display->NeedCommit = true;
		SetSelection(first_sel);
		display->Update();
	}
	// Always redraw, since the display is different in splitting mode
	box->SetKaraokeButtons();
	Refresh(false);

	LOG_D("karaoke/audio") << "returning";
}

/// @brief Split a syllable using the pending_slits data 
/// @param n 
/// @return 
///
int AudioKaraoke::SplitSyl (unsigned int n) {
	LOG_D("karaoke/audio") << "splitsyl, n=" << n;

	// Avoid multiple vector resizes (this must be first)
	syllables.reserve(syllables.size() + syllables[n].pending_splits.size());

	// The syllable we're splitting
	AudioKaraokeSyllable &basesyl = syllables[n];
	LOG_D("karaoke/audio") << "basesyl, contents='" << basesyl.unstripped_text.c_str() << "', selected=" << basesyl.selected;

	// Start by sorting the split points
	std::sort(basesyl.pending_splits.begin(), basesyl.pending_splits.end());

	wxString originalText = basesyl.text;
	int originalDuration = basesyl.duration;

	// Fixup the first syllable
	basesyl.text = originalText.Mid(0, basesyl.pending_splits[0] + 1);
	basesyl.unstripped_text = basesyl.text;
	basesyl.selected = false;
	basesyl.duration = originalDuration * basesyl.text.Length() / originalText.Length();
	int curpos = basesyl.start_time + basesyl.duration;

	// For each split, make a new syllable
	for (unsigned int i = 0; i < basesyl.pending_splits.size(); i++) {
		AudioKaraokeSyllable newsyl;
		if (i < basesyl.pending_splits.size()-1) {
			// in the middle
			newsyl.text = originalText.Mid(basesyl.pending_splits[i]+1, basesyl.pending_splits[i+1] - basesyl.pending_splits[i]);
		} else {
			// the last one (take the rest)
			newsyl.text = originalText.Mid(basesyl.pending_splits[i]+1);
		}
		newsyl.unstripped_text = newsyl.text;
		newsyl.duration = originalDuration * newsyl.text.Length() / originalText.Length();
		newsyl.start_time = curpos;
		newsyl.type = basesyl.type;
		newsyl.selected = false;//basesyl.selected;
		LOG_D("karaoke/audio") << "splitsyl: newsyl, contents='" << newsyl.text.c_str() << "', selected=" << newsyl.selected;
		curpos += newsyl.duration;
		syllables.insert(syllables.begin()+n+i+1, newsyl);
	}

	// The total duration of the new syllables will be equal to or less than the original duration
	// Fix this, so they'll always add up
	// Use an unfair method, just adding 1 to each syllable one after another, until it's correct
	int newDuration = 0;
	for (unsigned int j = n; j < basesyl.pending_splits.size()+n+1; j++) {
		newDuration += syllables[j].duration;
	}
	unsigned int k = n;
	while (newDuration < originalDuration) {
		syllables[k].duration++;
		k++;
		if (k >= syllables.size()) {
			k = n;
		}
		newDuration++;
	}

	// Prepare for return and clear pending splits
	int numsplits = basesyl.pending_splits.size();
	basesyl.pending_splits.clear();
	return numsplits;
}

/// @brief Apply delta length to syllable 
/// @param n     
/// @param delta 
/// @param mode  
/// @return 
///
bool AudioKaraoke::SyllableDelta(int n,int delta,int mode) {
	LOG_D("karaoke/audio") << "n=" << n << ", delta=" << delta << ", mode=" << mode;
	// Get syllable and next
	AudioKaraokeSyllable *curSyl=NULL,*nextSyl=NULL;
	curSyl = &syllables.at(n);
	int nkar = syllables.size();
	if (n < nkar-1) {
		nextSyl = &syllables.at(n+1);
	}

	// Get variables
	int len = curSyl->duration;

	// Cap delta
	int minLen = 0;
	if (len + delta < minLen) delta = minLen-len;
	if (mode == 0 && nextSyl && (nextSyl->duration - delta) < minLen) delta = nextSyl->duration - minLen;

	LOG_D("karaoke/audio") << "nkar=" << nkar << ", len=" << len << ", minLen=" << minLen << ", delta=" << delta;

	// Apply
	if (delta != 0) {
		LOG_D("karaoke/audio") << "delta != 0";
		curSyl->duration += delta;

		// Normal mode
		if (mode == 0 && nextSyl) {
			LOG_D("karaoke/audio") << "normal mode";
			nextSyl->duration -= delta;
			nextSyl->start_time += delta;
		}

		// Shift mode
		if (mode == 1) {
			LOG_D("karaoke/audio") << "shift mode";
			for (int i=n+1;i<nkar;i++) {
				syllables.at(i).start_time += delta;
			}
		}

		// Flag update
		LOG_D("karaoke/audio") << "return true";
		return true;
	}
	LOG_D("karaoke/audio") << "return false";
	return false;
}

/// @brief Karaoke tag menu constructor 
/// @param _kara 
///
AudioKaraokeTagMenu::AudioKaraokeTagMenu(AudioKaraoke *_kara)
: wxMenu(_("Karaoke tag"))
, kara(_kara)
{
	// Create menu items
	AppendCheckItem(10001, _T("\\k"), _("Change karaoke tag to \\k"));
	AppendCheckItem(10002, _T("\\kf / \\K"), _("Change karaoke tag to \\kf"));
	AppendCheckItem(10003, _T("\\ko"), _("Change karaoke tag to \\ko"));

	// Find out what kinds of tags are in use atm
	for (size_t i = 0; i < kara->syllables.size(); i++) {
		AudioKaraokeSyllable &syl = kara->syllables[i];
		if (syl.selected) {
			if (syl.type == _T("\\k")) {
				Check(10001, true);
			} else if (syl.type == _T("\\kf") || syl.type == _T("\\K")) {
				Check(10002, true);
			} else if (syl.type == _T("\\ko")) {
				Check(10003, true);
			}
		}
	}
}

/// @brief Karaoke tag menu destructor 
///
AudioKaraokeTagMenu::~AudioKaraokeTagMenu() {
}

///////////////
// Event table
BEGIN_EVENT_TABLE(AudioKaraokeTagMenu,wxMenu)
	EVT_MENU_RANGE(10001, 10003, AudioKaraokeTagMenu::OnSelectItem)
END_EVENT_TABLE()

/// @brief Karaoke tag menu event handler 
/// @param event 
///
void AudioKaraokeTagMenu::OnSelectItem(wxCommandEvent &event) {
	// Select the new tag for the syllables
	wxString newtag;
	switch (event.GetId()) {
		case 10001: newtag = _T("\\k"); break;
		case 10002: newtag = _T("\\kf"); break;
		case 10003: newtag = _T("\\ko"); break;
		default: return;
	}

	// Apply it
	size_t firstsel = kara->syllables.size();
	int lastsel = -1;
	for (size_t i = 0; i < kara->syllables.size(); i++) {
		AudioKaraokeSyllable &syl = kara->syllables[i];
		if (syl.selected) {
			if (firstsel > i) firstsel = i;
			lastsel = i;
			syl.type = newtag;
		}
	}

	// Update display
	kara->must_rebuild = true;
	//kara->Commit();
	//kara->display->NeedCommit = true;
	/// @todo Commit changes and stay on current line
	//kara->display->CommitChanges();
	//kara->display->Update();
	kara->SetSelection(firstsel, lastsel);
}

