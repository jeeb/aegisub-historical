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

/// @file video_slider.h
/// @see video_slider.cpp
/// @ingroup custom_control
///

#ifndef AGI_PRE
#include <vector>

#include <wx/window.h>
#endif

#include <libaegisub/signal.h>

namespace agi { struct Context; }

class VideoContext;
class SubtitlesGrid;

/// DOCME
/// @class VideoSlider
/// @brief DOCME
///
/// DOCME
class VideoSlider: public wxWindow {
	agi::Context *c;
	std::vector<int> keyframes; ///< Currently loaded keyframes
	std::vector<agi::signal::Connection> slots;

	int val; ///< Current frame number
	int max; ///< Last frame number

	/// Get the frame number for the given x coordinate
	int GetValueAtX(int x);
	/// Get the x-coordinate for a frame number
	int GetXAtValue(int value);
	/// Render the slider
	void DrawImage(wxDC &dc);
	/// Set the position of the slider
	void SetValue(int value);

	/// Video open event handler
	void VideoOpened();
	/// Keyframe open even handler
	void KeyframesChanged(std::vector<int> const& newKeyframes);

	void OnMouse(wxMouseEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnPaint(wxPaintEvent &event);
	void OnFocus(wxFocusEvent &event);
	void OnEraseBackground(wxEraseEvent &event) {}

public:
	VideoSlider(wxWindow* parent, agi::Context *c);

	DECLARE_EVENT_TABLE()
};
