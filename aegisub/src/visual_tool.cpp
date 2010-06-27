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

/// @file visual_tool.cpp
/// @brief Base class for visual typesetting functions
/// @ingroup visual_ts

#include "config.h"

#ifndef AGI_PRE
#include <algorithm>
#include <wx/glcanvas.h>
#endif

#ifdef __APPLE__
#include <OpenGL/GL.h>
#else
#include <GL/gl.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "ass_time.h"
#include "export_visible_lines.h"
#include "main.h"
#include "options.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "utils.h"
#include "vfr.h"
#include "video_context.h"
#include "video_display.h"
#include "video_provider_manager.h"
#include "visual_feature.h"
#include "visual_tool.h"
#include "visual_tool_clip.h"
#include "visual_tool_drag.h"
#include "visual_tool_vector_clip.h"

const wxColour IVisualTool::colour[4] = {wxColour(106,32,19), wxColour(255,169,40), wxColour(255,253,185), wxColour(187,0,0)};

template<class FeatureType>
VisualTool<FeatureType>::VisualTool(VideoDisplay *parent, VideoState const& video)
: realtime(OPT_GET("Video/Visual Realtime"))
, dragStartX(0)
, dragStartY(0)
, selChanged(false)
, parent(parent)
, holding(false)
, curDiag(NULL)
, dragging(false)
, externalChange(true)
, curFeature(NULL)
, dragListOK(false)
, frame_n(0)
, video(video)
, leftClick(false)
, leftDClick(false)
, shiftDown(false)
, ctrlDown(false)
, altDown(false)
{
	if (VideoContext::Get()->IsLoaded()) {
		frame_n = VideoContext::Get()->GetFrameN();
		VideoContext::Get()->grid->AddSelectionListener(this);
	}

	PopulateFeatureList();
}

template<class FeatureType>
VisualTool<FeatureType>::~VisualTool() {
	VideoContext::Get()->grid->RemoveSelectionListener(this);
}

template<class FeatureType>
void VisualTool<FeatureType>::OnMouseEvent (wxMouseEvent &event) {
	bool realTime = realtime->GetBool();
	bool needRender = false;

	if (event.Leaving()) {
		Update();
		parent->Render();
		return;
	}
	else if (event.Entering() && !OPT_GET("Tool/Visual/Always Show")->GetBool()) {
		needRender = true;
	}
	externalChange = false;

	leftClick = event.ButtonDown(wxMOUSE_BTN_LEFT);
	leftDClick = event.LeftDClick();
	shiftDown = event.m_shiftDown;
#ifdef __APPLE__
	ctrlDown = event.m_metaDown; // Cmd key
#else
	ctrlDown = event.m_controlDown;
#endif
	altDown = event.m_altDown;

	if (!dragListOK) {
		PopulateFeatureList();
		dragListOK = true;
	}
	if (!dragging) {
		unsigned oldHigh = curFeatureI;
		GetHighlightedFeature();
		if (curFeatureI != oldHigh) needRender = true;
	}

	if (dragging) {
		// continue drag
		if (event.LeftIsDown()) {
			for (selection_iterator cur = selFeatures.begin(); cur != selFeatures.end(); ++cur) {
				features[*cur].x = (video.x - dragStartX + features[*cur].origX);
				features[*cur].y = (video.y - dragStartY + features[*cur].origY);
				if (shiftDown) {
					if (abs(video.x - dragStartX) > abs(video.y - dragStartY)) {
						features[*cur].y = features[*cur].origY;
					}
					else {
						features[*cur].x = features[*cur].origX;
					}
				}
				UpdateDrag(&features[*cur]);

				if (realTime) {
					CommitDrag(&features[*cur]);
				}
			}
			if (realTime) {
				Commit();
			}
			else {
				needRender = true;
			}
		}
		// end drag
		else {
			if (realTime) AssLimitToVisibleFilter::SetFrame(-1);

			dragging = false;

			// mouse didn't move, fiddle with selection
			if (curFeature->x == curFeature->origX && curFeature->y == curFeature->origY) {
				// Don't deselect stuff that was selected in this click's mousedown event
				if (!selChanged) {
					if (ctrlDown) {
						// deselect this feature
						RemoveSelection(curFeatureI);
					}
					else {
						// deselect everything else
						ClearSelection();
						AddSelection(curFeatureI);
					}
					SetEditbox();
				}
			}
			else {
				for (selection_iterator cur = selFeatures.begin(); cur != selFeatures.end(); ++cur) {
					CommitDrag(&features[*cur]);
				}
				Commit(true);
			}

			curFeature = NULL;
			parent->ReleaseMouse();
			parent->SetFocus();
		}
	}
	else if (holding) {
		// continue hold
		if (event.LeftIsDown()) {
			UpdateHold();

			if (realTime) {
				CommitHold();
				Commit();
			}
			else {
				needRender = true;
			}
		}
		// end hold
		else {
			if (realTime) AssLimitToVisibleFilter::SetFrame(-1);

			holding = false;
			CommitHold();
			Commit(true);

			curDiag = NULL;
			parent->ReleaseMouse();
			parent->SetFocus();
		}
	}
	else if (leftClick) {
		// start drag
		if (curFeature) {
			if (InitializeDrag(curFeature)) {
				if (selFeatures.find(curFeatureI) == selFeatures.end()) {
					selChanged = true;
					if (!ctrlDown) {
						ClearSelection();
					}
					AddSelection(curFeatureI);
				}
				else {
					selChanged = false;
				}
				SetEditbox(curFeature->lineN);

				dragStartX = video.x;
				dragStartY = video.y;
				for (selection_iterator cur = selFeatures.begin(); cur != selFeatures.end(); ++cur) {
					features[*cur].origX = features[*cur].x;
					features[*cur].origY = features[*cur].y;
				}

				dragging = true;
				parent->CaptureMouse();
				if (realTime) AssLimitToVisibleFilter::SetFrame(frame_n);
			}
		}
		// start hold
		else {
			if (!altDown) {
				ClearSelection();
				SetEditbox();
				needRender = true;
			}
			curDiag = GetActiveDialogueLine();
			if (curDiag && InitializeHold()) {
				holding = true;
				parent->CaptureMouse();
				if (realTime) AssLimitToVisibleFilter::SetFrame(frame_n);
			}
		}
	}

	if (Update() || needRender) parent->Render();
	externalChange = true;
}

template<class FeatureType>
void VisualTool<FeatureType>::Commit(bool full, wxString message) {
	SubtitlesGrid *grid = VideoContext::Get()->grid;
	if (full) {
		if (message.empty()) {
			message = _("visual typesetting");
		}
		grid->ass->FlagAsModified(message);
	}
	grid->CommitChanges(false,!full);
	if (full)
		grid->editBox->Update(false, true, false);
}

template<class FeatureType>
AssDialogue* VisualTool<FeatureType>::GetActiveDialogueLine() {
	SubtitlesGrid *grid = VideoContext::Get()->grid;
	AssDialogue *diag = grid->GetActiveLine();
	if (grid->IsDisplayed(diag))
		return diag;
	return NULL;
}

template<class FeatureType>
void VisualTool<FeatureType>::GetHighlightedFeature() {
	int highestLayerFound = INT_MIN;
	curFeature = NULL;
	curFeatureI = -1;
	unsigned i = 0;
	for (feature_iterator cur = features.begin(); cur != features.end(); ++cur, ++i) {
		if (cur->IsMouseOver(video.x, video.y) && cur->layer > highestLayerFound) {
			curFeature = &*cur;
			curFeatureI = i;
			highestLayerFound = cur->layer;
		}
	}
}

template<class FeatureType>
void VisualTool<FeatureType>::DrawAllFeatures() {
	if (!dragListOK) {
		PopulateFeatureList();
		dragListOK = true;
	}

	SetLineColour(colour[0],1.0f,2);
	for (unsigned i = 0; i < features.size(); ++i) {
		int fill;
		if (&features[i] == curFeature)
			fill = 2;
		else if (selFeatures.find(i) != selFeatures.end())
			fill = 3;
		else
			fill = 1;
		SetFillColour(colour[fill],0.6f);
		features[i].Draw(*this);
	}
}

template<class FeatureType>
void VisualTool<FeatureType>::Refresh() {
	frame_n = VideoContext::Get()->GetFrameN();
	if (externalChange) {
		dragListOK = false;
		DoRefresh();
	}
}
template<class FeatureType>
void VisualTool<FeatureType>::AddSelection(unsigned i) {
	assert(i < features.size());

	if (selFeatures.insert(i).second && features[i].line) {
		lineSelCount[features[i].lineN] += 1;

		SubtitlesGrid *grid = VideoContext::Get()->grid;
		grid->SelectRow(features[i].lineN, true);
	}
}

template<class FeatureType>
void VisualTool<FeatureType>::RemoveSelection(unsigned i) {
	assert(i < features.size());

	if (selFeatures.erase(i) > 0 && features[i].line) {
		// Deselect a line only if all features for that line have been
		// deselected
		int lineN = features[i].lineN;
		lineSelCount[lineN] -= 1;
		assert(lineSelCount[lineN] >= 0);
		if (lineSelCount[lineN] <= 0) {
			SubtitlesGrid *grid = VideoContext::Get()->grid;
			grid->SelectRow(lineN, true, false);

			// We may have just deselected the active line, so make sure the
			// edit box is set to something sane
			SetEditbox();
		}
	}
}

template<class FeatureType>
wxArrayInt VisualTool<FeatureType>::GetSelection() {
	return VideoContext::Get()->grid->GetSelection();
}


template<class FeatureType>
void VisualTool<FeatureType>::ClearSelection(bool hard) {
	if (hard) {
		Selection sel;
		VideoContext::Get()->grid->SetSelectedSet(sel);
	}
	selFeatures.clear();
	lineSelCount.clear();
}

template<class FeatureType>
void VisualTool<FeatureType>::SetEditbox(int lineN) {
	VideoContext* con = VideoContext::Get();
	if (lineN > -1) {
		con->grid->SetActiveLine(con->grid->GetDialogue(lineN));
		con->grid->SelectRow(lineN, true);
	}
	else {
		Selection sel;
		con->grid->GetSelectedSet(sel);
		// If there is a selection and the edit box's line is in it, do nothing
		// Otherwise set the edit box if there is a selection or the selection
		// to the edit box if there is no selection
		if (sel.empty()) {
			sel.insert(con->grid->GetActiveLine());
			con->grid->SetSelectedSet(sel);
		}
		else if (sel.find(con->grid->GetActiveLine()) == sel.end()) {
			con->grid->SetActiveLine(con->grid->GetDialogue(con->grid->GetFirstSelRow()));
		}
	}
}

enum TagFoundType {
	TAG_NOT_FOUND = 0,
	PRIMARY_TAG_FOUND,
	ALT_TAG_FOUND
};

/// @brief Get the first value set for a tag
/// @param line Line to get the value from
/// @param tag  Tag to get the value of
/// @param n    Number of parameters passed
/// @return     Which tag (if any) was found
template<class T>
static TagFoundType get_value(const AssDialogue *line, wxString tag, size_t n, ...) {
	wxString alt;
	if (tag == L"\\pos") alt = L"\\move";
	else if (tag == L"\\an") alt = L"\\a";
	else if (tag == L"\\clip") alt = L"\\iclip";

	for (size_t i = 0; i < line->Blocks.size(); i++) {
		const AssDialogueBlockOverride *ovr = dynamic_cast<const AssDialogueBlockOverride*>(line->Blocks[i]);
		if (!ovr) continue;

		for (size_t j=0; j < ovr->Tags.size(); j++) {
			const AssOverrideTag *cur = ovr->Tags[j];
			if ((cur->Name == tag || cur->Name == alt) && cur->Params.size() >= n) {
				va_list argp;
				va_start(argp, n);
				for (size_t j = 0; j < n; j++) {
					T *val = va_arg(argp, T *);
					*val = cur->Params[j]->Get<T>(*val);
				}
				va_end(argp);
				return cur->Name == alt ? ALT_TAG_FOUND : PRIMARY_TAG_FOUND;
			}
		}
	}
	return TAG_NOT_FOUND;
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLinePosition(AssDialogue *diag,int &x, int &y) {
	int orgx,orgy;
	GetLinePosition(diag,x,y,orgx,orgy);
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLinePosition(AssDialogue *diag,int &x, int &y, int &orgx, int &orgy) {
	int margin[4];
	for (int i=0;i<4;i++) margin[i] = diag->Margin[i];
	int align = 2;

	AssStyle *style = VideoContext::Get()->grid->ass->GetStyle(diag->Style);
	if (style) {
		align = style->alignment;
		for (int i=0;i<4;i++) {
			if (margin[i] == 0) margin[i] = style->Margin[i];
		}
	}

	int sw,sh;
	VideoContext::Get()->GetScriptSize(sw,sh);

	// Process margins
	margin[1] = sw - margin[1];
	margin[3] = sh - margin[2];

	// Overrides processing
	diag->ParseASSTags();

	if (!get_value<int>(diag, "\\pos", 2, &x, &y)) {
		if (get_value<int>(diag, "\\an", 1, &align) == ALT_TAG_FOUND) {
			switch(align) {
				case 1: case 2: case 3:
					break;
				case 5: case 6: case 7:
					align += 2;
					break;
				case 9: case 10: case 11:
					align -= 5;
					break;
				default:
					align = 2;
					break;
			}
		}
		// Alignment type
		int hor = (align - 1) % 3;
		int vert = (align - 1) / 3;

		// Calculate positions
		if (hor == 0) x = margin[0];
		else if (hor == 1) x = (margin[0] + margin[1])/2;
		else if (hor == 2) x = margin[1];
		if (vert == 0) y = margin[3];
		else if (vert == 1) y = (margin[2] + margin[3])/2;
		else if (vert == 2) y = margin[2];
	}

	parent->FromScriptCoords(&x, &y);

	if (!get_value<int>(diag, "\\org", 2, &orgx, &orgy)) {
		orgx = x;
		orgy = y;
	}
	else {
		parent->FromScriptCoords(&orgx, &orgy);
	}

	diag->ClearBlocks();
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLineMove(AssDialogue *diag,bool &hasMove,int &x1,int &y1,int &x2,int &y2,int &t1,int &t2) {
	diag->ParseASSTags();

	hasMove =
		get_value<int>(diag, "\\move", 6, &x1, &y1, &x2, &y2, &t1, &t2) ||
		get_value<int>(diag, "\\move", 4, &x1, &y1, &x2, &y2);

	if (hasMove) {
		parent->FromScriptCoords(&x1, &y1);
		parent->FromScriptCoords(&x2, &y2);
	}

	diag->ClearBlocks();
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLineRotation(AssDialogue *diag,float &rx,float &ry,float &rz) {
	rx = ry = rz = 0.f;

	AssStyle *style = VideoContext::Get()->grid->ass->GetStyle(diag->Style);
	if (style) {
		rz = style->angle;
	}

	diag->ParseASSTags();

	get_value<double>(diag, L"\\frx", 1, &rx);
	get_value<double>(diag, L"\\fry", 1, &ry);
	get_value<double>(diag, L"\\frz", 1, &rz);

	diag->ClearBlocks();
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLineScale(AssDialogue *diag,float &scalX,float &scalY) {
	scalX = scalY = 100.f;

	AssStyle *style = VideoContext::Get()->grid->ass->GetStyle(diag->Style);
	if (style) {
		scalX = style->scalex;
		scalY = style->scaley;
	}

	diag->ParseASSTags();

	get_value<double>(diag, L"\\fscx", 1, &scalX);
	get_value<double>(diag, L"\\fscy", 1, &scalY);

	diag->ClearBlocks();
}

template<class FeatureType>
void VisualTool<FeatureType>::GetLineClip(AssDialogue *diag,int &x1,int &y1,int &x2,int &y2,bool &inverse) {
	x1 = y1 = 0;
	int sw,sh;
	VideoContext::Get()->GetScriptSize(sw,sh);
	x2 = sw-1;
	y2 = sh-1;
	inverse = false;

	diag->ParseASSTags();
	inverse = get_value<int>(diag, L"\\clip", 4, &x1, &y1, &x2, &y2) == ALT_TAG_FOUND;
	diag->ClearBlocks();

	parent->FromScriptCoords(&x1, &y1);
	parent->FromScriptCoords(&x2, &y2);
}

template<class FeatureType>
wxString VisualTool<FeatureType>::GetLineVectorClip(AssDialogue *diag,int &scale,bool &inverse) {
	scale = 1;
	inverse = false;
	diag->ParseASSTags();

	int x1, y1, x2, y2;
	TagFoundType res = get_value<int>(diag, L"\\clip", 4, &x1, &y1, &x2, &y2);
	if (res) {
		inverse = res == ALT_TAG_FOUND;
		diag->ClearBlocks();
		return wxString::Format(L"m %d %d l %d %d %d %d %d %d", x1, y1, x2, y1, x2, y2, x1, y2);
	}
	wxString result;
	wxString scaleStr;
	res = get_value<wxString>(diag, L"\\clip", 2, &scaleStr, &result);
	inverse = res == ALT_TAG_FOUND;
	if (!scaleStr.empty()) {
		long s;
		scaleStr.ToLong(&s);
		scale = s;
	}
	diag->ClearBlocks();
	return result;
}

/// @brief Set override 
/// @param tag   
/// @param value 
template<class FeatureType>
void VisualTool<FeatureType>::SetOverride(AssDialogue* line, wxString tag, wxString value) {
	if (!line) return;

	wxString removeTag;
	if (tag == L"\\1c") removeTag = L"\\c";
	else if (tag == L"\\fr") removeTag = L"\\frz";
	else if (tag == L"\\pos") removeTag = L"\\move";
	else if (tag == L"\\move") removeTag = L"\\pos";
	else if (tag == L"\\clip") removeTag = L"\\iclip";
	else if (tag == L"\\iclip") removeTag = L"\\clip";

	wxString insert = tag + value;

	// Get block at start
	line->ParseASSTags();
	AssDialogueBlock *block = line->Blocks.at(0);

	// Get current block as plain or override
	AssDialogueBlockPlain *plain = dynamic_cast<AssDialogueBlockPlain*>(block);
	AssDialogueBlockOverride *ovr = dynamic_cast<AssDialogueBlockOverride*>(block);
	assert(dynamic_cast<AssDialogueBlockDrawing*>(block) == NULL);

	if (plain) {
		line->Text = L"{" + insert + L"}" + line->Text;
	}
	else if (ovr) {
		// Remove old of same
		for (size_t i = 0; i < ovr->Tags.size(); i++) {
			wxString name = ovr->Tags[i]->Name;
			if (tag == name || removeTag == name) {
				delete ovr->Tags[i];
				ovr->Tags.erase(ovr->Tags.begin() + i);
				i--;
			}
		}
		ovr->AddTag(insert);

		line->UpdateText();
	}

	parent->SetFocus();
}

// If only export worked
template class VisualTool<VisualDraggableFeature>;
template class VisualTool<ClipCorner>;
template class VisualTool<VisualToolDragDraggableFeature>;
template class VisualTool<VisualToolVectorClipDraggableFeature>;
