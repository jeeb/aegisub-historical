// Copyright (c) 2005, Niels Martin Hansen
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

/// @file export_clean_info.cpp
/// @brief Clean Script Info export filter
/// @ingroup export
///

#include "config.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "export_clean_info.h"

AssTransformCleanInfoFilter::AssTransformCleanInfoFilter()
: AssExportFilter(_("Clean Script Info"), _("Removes all but the absolutely required fields from the Script Info section. You might want to run this on files that you plan to distribute in original form."))
{
}

void AssTransformCleanInfoFilter::ProcessSubs(AssFile *subs, wxWindow *) {
	using std::list;
	AssEntry *curEntry;
	entryIter cur, next = subs->Line.begin();
	while (next != subs->Line.end()) {
		cur = next++;

		curEntry = *cur;
		if (curEntry->group != "[Script Info]") {
			continue;
		}
		if (curEntry->GetEntryData().empty()) {
			continue;
		}
		if (curEntry->GetEntryData() == "[Script Info]") {
			continue;
		}
		if (curEntry->GetEntryData().Left(1) == ";") {
			continue;
		}

		wxString field = curEntry->GetEntryData().Left(curEntry->GetEntryData().Find(_T(':'))).Lower();
		if (field != "scripttype" &&
			field != "collisions" &&
			field != "playresx" &&
			field != "playresy" &&
			field != "wrapstyle" &&
			field != "scaledborderandshadow") {
			delete curEntry;
			subs->Line.erase(cur);
		}
	}
}
