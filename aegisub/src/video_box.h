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

/// @file video_box.h
/// @see video_box.cpp
/// @ingroup main_ui video
///

#ifndef AGI_PRE
#include <list>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/toolbar.h>
#endif

#include <libaegisub/signal.h>

namespace agi { struct Context; }
class ToggleBitmap;
class VideoDisplay;
class VideoSlider;
class wxComboBox;

/// DOCME
/// @class VideoBox
/// @brief DOCME
///
/// DOCME
class VideoBox : public wxPanel {
	std::list<agi::signal::Connection> slots;
	agi::Context *context;     ///< Project context
	wxTextCtrl *VideoPosition; ///< Current frame/time
	wxTextCtrl *VideoSubsPos;  ///< Time relative to the active subtitle line
	wxComboBox *zoomBox;

	/// Handle a click on the play/pause buttons
	void OnButton(wxCommandEvent &evt);

	/// Update VideoPosition and VideoSubsPos
	void UpdateTimeBoxes();
public:

	/// DOCME
	wxToolBar *visualToolBar;

	/// DOCME
	wxToolBar *visualSubToolBar;

	/// DOCME
	VideoDisplay *videoDisplay;

	/// DOCME
	VideoSlider *videoSlider;

	VideoBox(wxWindow *parent, bool isDetached, agi::Context *context);
};

// IDs
enum {
	Video_Mode_Standard = 5000,
	Video_Mode_Drag,
	Video_Mode_Rotate_Z,
	Video_Mode_Rotate_XY,
	Video_Mode_Scale,
	Video_Mode_Clip,
	Video_Mode_Vector_Clip,
};
