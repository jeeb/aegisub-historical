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

/// @file dialog_shift_times.cpp
/// @brief Shift Times dialogue box and logic
/// @ingroup secondary_ui
///

#include "config.h"

#ifndef AGI_PRE
#include <algorithm>
#include <fstream>
#include <string>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_time.h"
#include "charset_conv.h"
#include "dialog_shift_times.h"
#include "include/aegisub/context.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "standard_paths.h"
#include "subs_grid.h"
#include "timeedit_ctrl.h"
#include "utils.h"
#include "video_context.h"

// IDs
enum {
	TEXT_SHIFT_TIME = 1100,
	TEXT_SHIFT_FRAME,
	RADIO_BACKWARD,
	RADIO_FORWARD,
	RADIO_TIME,
	RADIO_FRAME,
	SHIFT_CLEAR_HISTORY
};

DialogShiftTimes::DialogShiftTimes (agi::Context *context)
: wxDialog(context->parent, -1, _("Shift Times"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("JumpTo"))
, context(context)
{
	// Set icon
	SetIcon(BitmapToIcon(GETIMAGE(shift_times_toolbutton_24)));

	// Set initial values
	ready = true;
	shiftframe = 0;
	
	// Static-box sizers before anything else
	wxSizer *TimesSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Shift by"));
	wxSizer *HistorySizer = new wxStaticBoxSizer(wxVERTICAL,this,_("History"));

	// Times
	RadioTime = new wxRadioButton(this,RADIO_TIME,_("Time: "),wxDefaultPosition,wxDefaultSize, wxRB_GROUP);
	RadioFrames = new wxRadioButton(this,RADIO_FRAME,_("Frames: "),wxDefaultPosition,wxDefaultSize);
	ShiftTime = new TimeEdit(this,TEXT_SHIFT_TIME,_T(""),wxDefaultPosition,wxDefaultSize);
	ShiftFrame = new wxTextCtrl(this,TEXT_SHIFT_FRAME,wxString::Format(_T("%i"),shiftframe),wxDefaultPosition,wxDefaultSize);
	ShiftTime->SetToolTip(_("Enter time in h:mm:ss.cs notation"));
	RadioTime->SetToolTip(_("Shift by time"));
	ShiftFrame->Disable();
	if (!context->videoController->TimecodesLoaded()) RadioFrames->Disable();
	else {
		ShiftFrame->SetToolTip(_("Enter number of frames to shift by"));
		RadioFrames->SetToolTip(_("Shift by frames"));
	}
	wxSizer *TimeFrameSizer = new wxFlexGridSizer(2,2,5,5);
	TimeFrameSizer->Add(RadioTime,0,wxALIGN_CENTER_VERTICAL,0);
	TimeFrameSizer->Add(ShiftTime,1);
	TimeFrameSizer->Add(RadioFrames,0,wxALIGN_CENTER_VERTICAL,0);
	TimeFrameSizer->Add(ShiftFrame,1);

	// Direction
	DirectionForward = new wxRadioButton(this,-1,_("Forward"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	DirectionBackward = new wxRadioButton(this,-1,_("Backward"), wxDefaultPosition, wxDefaultSize);
	DirectionForward->SetToolTip(_("Shifts subs forward, making them appear later. Use if they are appearing too soon."));
	DirectionBackward->SetToolTip(_("Shifts subs backward, making them appear earlier. Use if they are appearing too late."));
	wxSizer *DirectionSizer = new wxBoxSizer(wxHORIZONTAL);
	DirectionSizer->Add(DirectionForward,1,wxEXPAND,0);
	DirectionSizer->Add(DirectionBackward,1,wxLEFT | wxEXPAND,5);
	TimesSizer->Add(TimeFrameSizer,0,wxEXPAND,0);
	TimesSizer->Add(DirectionSizer,0,wxEXPAND | wxTOP,5);

	// Selection
	wxString SelChoices[3] = { _("All rows"), _("Selected rows"), _("Selection onward") };
	SelChoice = new wxRadioBox(this,-1,_("Affect"), wxDefaultPosition, wxDefaultSize, 3, SelChoices, 3, wxRA_SPECIFY_ROWS);

	// Times
	wxString TimesChoices[3] = { _("Start and End times"), _("Start times only"), _("End times only") };
	TimesChoice = new wxRadioBox(this,-1,_("Times"), wxDefaultPosition, wxDefaultSize, 3, TimesChoices, 3, wxRA_SPECIFY_ROWS);

	// History
	History = new wxListBox(this,-1,wxDefaultPosition,wxSize(350,100), 0, NULL, wxLB_HSCROLL);
	wxButton *ClearButton = new wxButton(this,SHIFT_CLEAR_HISTORY,_("Clear"));
	HistorySizer->Add(History,1,wxEXPAND,0);
	HistorySizer->Add(ClearButton,0,wxEXPAND,0);

	// Buttons
	wxStdDialogButtonSizer *ButtonSizer = new wxStdDialogButtonSizer();
	ButtonSizer->AddButton(new wxButton(this,wxID_OK));
	ButtonSizer->AddButton(new wxButton(this,wxID_CANCEL));
	ButtonSizer->AddButton(new HelpButton(this,_T("Shift Times")));
	ButtonSizer->Realize();

	// General layout
	wxSizer *LeftSizer = new wxBoxSizer(wxVERTICAL);
	wxSizer *RightSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer *TopSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	LeftSizer->Add(TimesSizer,0,wxEXPAND | wxBOTTOM,5);
	LeftSizer->Add(SelChoice,0,wxEXPAND | wxBOTTOM,5);
	LeftSizer->Add(TimesChoice,0,wxEXPAND,5);
	RightSizer->Add(HistorySizer,0,wxEXPAND,0);
	TopSizer->Add(LeftSizer,0,wxEXPAND | wxRIGHT,5);
	TopSizer->Add(RightSizer,0,wxEXPAND,0);
	MainSizer->Add(TopSizer,0,wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,5);
	MainSizer->Add(ButtonSizer,0,wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,5);
	
	// Set sizer
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	CenterOnParent();

	// Load values from options
	if (!OPT_GET("Tool/Shift Times/ByTime")->GetBool()) {
		if (RadioFrames->IsEnabled()) {
			RadioFrames->SetValue(true);
			ShiftFrame->Enable(true);
			ShiftTime->Enable(false);
			ShiftFrame->SetValue(AegiIntegerToString(OPT_GET("Tool/Shift Times/Length")->GetInt()));
		}
	}
	else {
		ShiftTime->SetTime(OPT_GET("Tool/Shift Times/Length")->GetInt());
	}
	TimesChoice->SetSelection(OPT_GET("Tool/Shift Times/Type")->GetInt());
	SelChoice->SetSelection(OPT_GET("Tool/Shift Times/Affect")->GetInt());
	if (OPT_GET("Tool/Shift Times/Direction")->GetBool()) DirectionBackward->SetValue(true);

	// Has selection?
	wxArrayInt sel = context->subsGrid->GetSelection();
	if (sel.Count() == 0) {
		SelChoice->Enable(1,false);
		SelChoice->Enable(2,false);
		SelChoice->SetSelection(0);
	}

	// Load history
	LoadHistory(StandardPaths::DecodePath(_T("?user/shift_history.txt")));
}


///////////////
// Event table
BEGIN_EVENT_TABLE(DialogShiftTimes, wxDialog)
	EVT_BUTTON(wxID_CANCEL,DialogShiftTimes::OnClose)
	EVT_BUTTON(wxID_OK,DialogShiftTimes::OnOK)
	EVT_BUTTON(SHIFT_CLEAR_HISTORY,DialogShiftTimes::OnClear)
	EVT_RADIOBUTTON(RADIO_TIME,DialogShiftTimes::OnRadioTime)
	EVT_RADIOBUTTON(RADIO_FRAME,DialogShiftTimes::OnRadioFrame)
END_EVENT_TABLE()



/// @brief Clear History 
/// @param event 
///
void DialogShiftTimes::OnClear(wxCommandEvent &event) {
	wxRemoveFile(StandardPaths::DecodePath(_T("?user/shift_history.txt")));
	History->Clear();
}


/// @brief Cancel 
/// @param event 
///
void DialogShiftTimes::OnClose(wxCommandEvent &event) {
	EndModal(0);
}



/// @brief Apply 
/// @param event 
/// @return 
///
void DialogShiftTimes::OnOK(wxCommandEvent &event) {
	// General values
	int type = TimesChoice->GetSelection();
	int affect = SelChoice->GetSelection();
	bool allrows = affect == 0;
	bool selOnward = affect == 2;
	long len;
	bool byTime = RadioTime->GetValue();
	bool backward = DirectionBackward->GetValue();
	bool didSomething = false;

	// Selection
	int nrows = context->subsGrid->GetRows();
	wxArrayInt sel = context->subsGrid->GetSelection();
	int firstSel = 0;
	if (sel.Count()) firstSel = sel[0];

	// Get length
	if (byTime) len = ShiftTime->time.GetMS();
	else ShiftFrame->GetValue().ToLong(&len);

	if (byTime && len == 0) {
		// Shift zero milliseconds in time mode
		// Equivalent to doing nothing at all, so just dismiss
		EndModal(0);
		return;
	}

	// If backwards, invert
	if (backward) len = -len;

	// Shift
	for (int i=0;i<nrows;i++) {
		if (allrows || (i >= firstSel && selOnward) || context->subsGrid->IsInSelection(i)) {
			if (byTime) context->subsGrid->ShiftLineByTime(i,len,type);
			else context->subsGrid->ShiftLineByFrames(i,len,type);
			didSomething = true;
		}
	}

	// Add entry to history
	if (didSomething) {
		if (backward) len = -len;
		wxString message;
		wxFileName assfile(context->ass->filename);
		wxString filename = assfile.GetFullName();

		// File
		if (filename.IsEmpty()) message << _("unsaved, ");
		else message << filename << _T(", ");

		// Time/frames
		if (byTime) message << ShiftTime->GetValue() << _T(" ");
		else message << len << _(" frames ");

		// Forward/backwards
		if (backward) message << _("backward, ");
		else message << _("forward, ");

		// Start/end
		if (type == 0) message << _("s+e, ");
		if (type == 1) message << _("s, ");
		if (type == 2) message << _("e, ");

		// Selection range
		if (affect == 0) message << _("all");
		else if (affect == 2) message << wxString::Format(_("from %i onward"),sel[0]+1);
		else {	// This huge block of code prints the selected ranges of subs
			message << _("sel ");
			int last = sel[0]-1;
			int first = sel[0];
			for (unsigned int i=0;i<sel.Count();i++) {
				if (sel[i] != last+1) {
					if (first != last) message << wxString::Format(_T("%i"),first+1) << _T("-") << wxString::Format(_T("%i"),last+1) << _T(";");
					else message << wxString::Format(_T("%i"),first+1) << _T(";");
					first = sel[i];
				}
				last = sel[i];
			}
			if (first != last) message << wxString::Format(_T("%i"),first+1) << _T("-") << wxString::Format(_T("%i"),last+1);
			else message << wxString::Format(_T("%i"),first+1);
		}

		// Done, append
		AppendToHistory(message);
	}

	// Store modifications
	OPT_SET("Tool/Shift Times/ByTime")->SetBool(byTime);
	OPT_SET("Tool/Shift Times/Type")->SetInt(type);
	OPT_SET("Tool/Shift Times/Length")->SetInt(len);
	OPT_SET("Tool/Shift Times/Affect")->SetInt(affect);
	OPT_SET("Tool/Shift Times/Direction")->SetBool(backward);

	// End dialog
	context->ass->Commit(_("shifting"), AssFile::COMMIT_TIMES);
	EndModal(0);
}



/// @brief Set to time 
/// @param event 
///
void DialogShiftTimes::OnRadioTime(wxCommandEvent &event) {
	ShiftTime->Enable(true);
	ShiftFrame->Enable(false);
	event.Skip();
}



/// @brief Set to frame 
/// @param event 
///
void DialogShiftTimes::OnRadioFrame(wxCommandEvent &event) {
	ShiftTime->Enable(false);
	ShiftFrame->Enable(true);
	event.Skip();
}



/// @brief Appends a line to history 
/// @param text 
/// @return 
///
void DialogShiftTimes::AppendToHistory(wxString text) {
	// Open file
	if (HistoryFile.IsEmpty()) return;
	using namespace std;
	ofstream file;
	file.open(HistoryFile.mb_str(csConvLocal),ios::out | ios::app);
	if (!file.is_open()) {
		return;
	}

	// Insert line
	file << text.mb_str(wxConvUTF8) << endl;

	// Close
	file.close();
}



/// @brief Loads history from disk 
/// @param filename 
///
void DialogShiftTimes::LoadHistory(wxString filename) {
	// Open file
	using namespace std;
	HistoryFile = filename;
	ifstream file;
	file.open(filename.mb_str(csConvLocal));
	if (!file.is_open()) {
		return;
	}

	// Setup
	string buffer;
	History->Clear();
	History->Freeze();

	// Get lines
	while (!file.eof()) {
		getline(file,buffer);
		wxString curLine(buffer.c_str(),wxConvUTF8);
		if (curLine != _T("")) History->Insert(curLine,0);
	}

	// Finish updating
	History->Thaw();
	//History->SetFirstItem(History->GetCount()-1);

	// Close
	file.close();
}


