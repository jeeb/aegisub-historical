// Copyright (c) 2008-2009, Karl Blomster
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

/// @file ffmpegsource_common.h
/// @see ffmpegsource_common.cpp
/// @ingroup video_input audio_input ffms
///

#ifdef WITH_FFMPEGSOURCE

#ifndef AGI_PRE
#include <map>

//#include <wx/filename.h>
//#include <wx/thread.h>
#endif

#include "../../libffms/include/ffms.h"

namespace media {
	namespace ffms {

//#include "dialog_progress.h"

/// Index all tracks
#define FFMS_TRACKMASK_ALL		-1
/// Index no tracks
#define FFMS_TRACKMASK_NONE		0

/// @class FFmpegSourceProvider
/// @brief Base class for FFMS2 source providers; contains common functions etc
class FFmpegSourceProvider {
public:
	/// Logging level constants from avutil/log.h
	enum FFMS_LogLevel {
		/// nothing printed
		FFMS_LOG_QUIET		= -8,
		FFMS_LOG_PANIC		= 0,
		FFMS_LOG_FATAL		= 8,
		FFMS_LOG_ERROR		= 16,
		FFMS_LOG_WARNING	= 24,
		FFMS_LOG_INFO		= 32,
		FFMS_LOG_VERBOSE	= 40,
		FFMS_LOG_DEBUG		= 48,
	};

	/// Indexing progress report dialog
	struct IndexingProgressDialog {
		volatile bool IndexingCanceled;
//		DialogProgress *ProgressDialog;
	};

	static int FFMS_CC UpdateIndexingProgress(int64_t Current, int64_t Total, void *Private);

	FFMS_Index *DoIndexing(FFMS_Indexer *Indexer, const std::string& Cachename, int Trackmask, FFMS_IndexErrorHandling IndexEH);
	std::map<int,std::string> GetTracksOfType(FFMS_Indexer *Indexer, FFMS_TrackType Type);
	int AskForTrackSelection(const std::map<int,std::string>& TrackList, FFMS_TrackType Type);
	std::string GetCacheFilename(const std::string& filename);
	void SetLogLevel();
	FFMS_IndexErrorHandling GetErrorHandlingMode();

	virtual ~FFmpegSourceProvider() {}
};

#endif /* WITH_FFMPEGSOURCE */

	} // namespace ffms
} // namespace media
