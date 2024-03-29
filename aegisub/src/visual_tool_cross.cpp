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

/// @file visual_tool_cross.cpp
/// @brief Crosshair double-click-to-position visual typesetting tool
/// @ingroup visual_ts

#include "config.h"

#include "ass_file.h"
#include "include/aegisub/context.h"
#include "gl_text.h"
#include "video_context.h"
#include "video_display.h"
#include "visual_tool_cross.h"

VisualToolCross::VisualToolCross(VideoDisplay *parent, agi::Context *context, VideoState const& video, wxToolBar *)
: VisualTool<VisualDraggableFeature>(parent, context, video)
, glText(new OpenGLText)
{
}

bool VisualToolCross::Update() {
	if (!leftDClick) return true;
	if (!curDiag) return true;

	int dx, dy;
	int vx = video.x;
	int vy = video.y;
	GetLinePosition(curDiag, dx, dy);
	parent->ToScriptCoords(&vx, &vy);
	parent->ToScriptCoords(&dx, &dy);
	dx -= vx;
	dy -= vy;

	Selection sel = c->selectionController->GetSelectedSet();
	for (Selection::const_iterator cur = sel.begin(); cur != sel.end(); ++cur) {
		int x1, y1;
		GetLinePosition(*cur, x1, y1);
		parent->ToScriptCoords(&x1, &y1);
		SetOverride(*cur, L"\\pos", wxString::Format(L"(%i,%i)", x1 - dx, y1 - dy));
	}

	Commit(_("positioning"));
	return false;
}

void VisualToolCross::Draw() {
	// Draw cross
	glDisable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	glLineWidth(1);
	glBegin(GL_LINES);
		glColor3f(1.0f,1.0f,1.0f);
		glVertex2f(0, video.y);
		glVertex2f(video.w, video.y);
		glVertex2f(video.x, 0);
		glVertex2f(video.x, video.h);
	glEnd();
	glDisable(GL_COLOR_LOGIC_OP);

	int tx,ty;
	if (!wxGetMouseState().ShiftDown()) {
		tx = video.x;
		ty = video.y;
	}
	else {
		tx = video.w - video.x;
		ty = video.h - video.y;
	}
	parent->ToScriptCoords(&tx, &ty);
	wxString mouseText = wxString::Format(L"%i,%i", tx, ty);

	int tw,th;
	glText->SetFont(L"Verdana", 12, true, false);
	glText->SetColour(wxColour(255, 255, 255), 1.f);
	glText->GetExtent(mouseText, tw, th);

	// Calculate draw position
	int dx = video.x;
	int dy = video.y;
	bool left = dx > video.w / 2;
	bool bottom = dy < video.h / 2;

	// Text draw coords
	if (left) dx -= tw + 4;
	else dx += 4;
	if (bottom) dy += 3;
	else dy -= th + 3;

	// Draw text
	glText->Print(mouseText, dx, dy);
}
