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

/// @file validators.cpp
/// @brief Various validators for wx
/// @ingroup custom_control utility
///

#include "config.h"

#ifndef AGI_PRE
#include <wx/textctrl.h>
#endif

#include "utils.h"
#include "validators.h"

NumValidator::NumValidator(wxString val, bool isfloat, bool issigned)
: isFloat(isfloat)
, isSigned(issigned)
{
	if (isFloat) {
		val.ToDouble(&fValue);
	}
	else {
		long tLong;
		val.ToLong(&tLong);
		iValue = tLong;
	}
}

NumValidator::NumValidator(const NumValidator &from)
: wxValidator()
, fValue(from.fValue)
, iValue(from.iValue)
, isFloat(from.isFloat)
, isSigned(from.isSigned)
{
	SetWindow(from.GetWindow());
}

BEGIN_EVENT_TABLE(NumValidator, wxValidator)
	EVT_CHAR(NumValidator::OnChar)
END_EVENT_TABLE()

wxObject* NumValidator::Clone() const {
	return new NumValidator(*this);
}

bool NumValidator::Validate(wxWindow* parent) {
	wxTextCtrl *ctrl = (wxTextCtrl*) GetWindow();
	wxString value = ctrl->GetValue();

	if (!ctrl->IsEnabled()) return true;

	if (value.Length() < 1) return false;

	bool gotDecimal = false;
	for (size_t i = 0; i < value.Length(); i++) {
		if (!CheckCharacter(value[i], !i, true, gotDecimal))
			return false;
	}
	return true;
}

bool NumValidator::CheckCharacter(int chr, bool isFirst, bool canSign, bool &gotDecimal) {
	// Check sign
	if (chr == '-' || chr == '+') {
		return isFirst && canSign && isSigned;
	}

	// Don't allow anything before a sign
	if (isFirst && !canSign) return false;

	// Check decimal point
	if (chr == '.' || chr == ',') {
		if (!isFloat || gotDecimal)
			return false;
		else {
			gotDecimal = true;
			return true;
		}
	}

	// Check digit
	return chr >= '0' && chr <= '9';
}

void NumValidator::OnChar(wxKeyEvent& event) {
	wxTextCtrl *ctrl = (wxTextCtrl*) GetWindow();
	wxString value = ctrl->GetValue();
	int chr = event.GetKeyCode();

	// Special keys
	if (chr < WXK_SPACE || chr == WXK_DELETE || chr > WXK_START) {
		event.Skip();
		return;
	}

	// Get selection
	long from,to;
	ctrl->GetSelection(&from,&to);

	// Count decimal points and signs outside selection
	int decimals = 0;
	int signs = 0;
	wxChar curchr;
	for (size_t i=0;i<value.Length();i++) {
		if (i >= (unsigned)from && i < (unsigned)to) continue;
		curchr = value[i];
		if (curchr == '.' || curchr == ',') decimals++;
		if (curchr == '+' || curchr == '-') signs++;
	}
	bool gotDecimal = decimals > 0;

	// Check character
	if (!CheckCharacter(chr,!from,!signs,gotDecimal)) {
		if (!wxValidator::IsSilent()) wxBell();
		return;
	}

	// OK
	event.Skip();
	return;
}

bool NumValidator::TransferToWindow() {
	wxTextCtrl *ctrl = (wxTextCtrl*) GetWindow();
	if (isFloat)
		ctrl->SetValue(wxString::Format("%g",fValue));
	else
		ctrl->SetValue(wxString::Format("%d",iValue));

	return true;
}

bool NumValidator::TransferFromWindow() {
	wxTextCtrl *ctrl = (wxTextCtrl*) GetWindow();
	wxString value = ctrl->GetValue();

	if (!Validate(ctrl)) return false;

	// Transfer
	if (isFloat) {
		value.ToDouble(&fValue);
	}
	else {
		long tLong;
		value.ToLong(&tLong);
		iValue = tLong;
	}

	return true;
}
