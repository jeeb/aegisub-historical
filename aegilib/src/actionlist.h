// Copyright (c) 2008, Rodrigo Braz Monteiro
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
// AEGISUB/ATHENASUB
//
// Website: http://www.aegisub.net
// Contact: mailto:amz@aegisub.net
//

#pragma once
#include <list>
#include "action.h"
#include "athenastring.h"
#include "section_entry.h"
#include "selection.h"
#include "api.h"
#include "model.h"
#include "interfaces.h"

namespace Athenasub {

	// Prototypes
	class CController;

	// ActionList class
	class CActionList : public IActionList {
		friend class CModel;
		friend class CController;

	private:
		String actionName;
		String owner;
		Model model;
		std::list<Action> actions;
		bool valid;
		bool undoAble;

		CActionList(Model model,const String actionName,const String owner,bool undoAble);
		void Start(const String actionName);
		void AddActionStart(const Action action);

	public:
		virtual ~CActionList();

		virtual String GetName() const { return actionName; }
		virtual String GetOwner() const { return owner; }

		virtual void AddAction(Action action);
		virtual void Finish();

		virtual void InsertLine(Entry line,int position=-1,const String section=L"");
		virtual void RemoveLine(int position,const String section);
		virtual Entry ModifyLine(int position,const String section);
		virtual std::vector<Entry> ModifyLines(Selection selection,const String section);
	};

}