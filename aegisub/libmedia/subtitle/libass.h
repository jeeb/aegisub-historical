// Copyright (c) 2006-2007, Rodrigo Braz Monteiro, Evgeniy Stepanov
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

/// @file subtitles_provider_libass.h
/// @see subtitles_provider_libass.cpp
/// @ingroup subtitle_rendering
///

#ifdef WITH_LIBASS

#include "include/aegisub/subtitles_provider.h"
extern "C" {
#ifdef __VISUALC__
#include "stdint.h"
#endif

#include "../libass/ass.h"
}

class FontConfigCacheThread;

/// DOCME
/// @class LibassSubtitlesProvider
/// @brief DOCME
///
/// DOCME
class LibassSubtitlesProvider : public SubtitlesProvider {
	/// DOCME
	static ASS_Library* ass_library;

	/// DOCME
	ASS_Renderer* ass_renderer;

	/// DOCME
	ASS_Track* ass_track;

	static FontConfigCacheThread *cache_worker;

public:
	LibassSubtitlesProvider(std::string);
	~LibassSubtitlesProvider();

	void LoadSubtitles(AssFile *subs);
	void DrawSubtitles(AegiVideoFrame &dst,double time);

	static void CacheFonts();
};
#endif
