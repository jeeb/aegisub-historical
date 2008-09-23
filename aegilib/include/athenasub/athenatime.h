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

namespace Athenasub {

	// Time class
	class Time {
	private:
		int ms;	// Time in milliseconds

	public:
		Time() { ms = 0; }
		Time(int milliseconds) { ms = milliseconds; }

		inline void SetMS(int milliseconds) { ms = milliseconds; }
		inline int GetMS() const { return ms; }
	};

	// Operators
	inline Time operator+  (const Time& p1,int p2)         { return Time(p1.GetMS()+p2); }
	inline Time operator-  (const Time& p1,int p2)         { return Time(p1.GetMS()-p2); }
	inline bool operator== (const Time& p1,const Time& p2) { return p1.GetMS() == p2.GetMS(); }
	inline bool operator!= (const Time& p1,const Time& p2) { return p1.GetMS() != p2.GetMS(); }
	inline bool operator<= (const Time& p1,const Time& p2) { return p1.GetMS() <= p2.GetMS(); }
	inline bool operator>= (const Time& p1,const Time& p2) { return p1.GetMS() >= p2.GetMS(); }
	inline bool operator<  (const Time& p1,const Time& p2) { return p1.GetMS() <  p2.GetMS(); }
	inline bool operator>  (const Time& p1,const Time& p2) { return p1.GetMS() >  p2.GetMS(); }
}