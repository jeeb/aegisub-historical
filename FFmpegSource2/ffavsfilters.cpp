//  Copyright (c) 2007-2008 Fredrik Mellbin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <string>
#include "ffms.h"
#include "ffavisynth.h"
#include "ffswscale.h"
#include "ffpp.h"

int GetNumberOfLogicalCPUs() {
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);
	return SI.dwNumberOfProcessors;
}

AVSValue __cdecl CreateFFIndex(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	FFMS_Init();

	char ErrorMsg[1024];
	unsigned MsgSize = sizeof(ErrorMsg);


	if (!Args[0].Defined())
    	Env->ThrowError("FFIndex: No source specified");

	const char *Source = Args[0].AsString();
	const char *CacheFile = Args[1].AsString("");
	int TrackMask = Args[2].AsInt(0);
	const char *AudioFile = Args[3].AsString("");
	bool OverWrite = Args[4].AsBool(false);

	std::string DefaultCache(Source);
	DefaultCache.append(".ffindex");
	if (!strcmp(CacheFile, ""))
		CacheFile = DefaultCache.c_str();

	if (!strcmp(AudioFile, ""))
		AudioFile = Source;

	// Return values
	// 0: Index already present
	// 1: Index generated
	// 2: Index forced to be overwritten

	FrameIndex *Index;
	if (OverWrite || !(Index = FFMS_ReadIndex(CacheFile, ErrorMsg, MsgSize))) {
		if (!(Index = FFMS_MakeIndex(Source, TrackMask, AudioFile, NULL, NULL, ErrorMsg, MsgSize)))
			Env->ThrowError("FFIndex: %s", ErrorMsg);
		if (FFMS_WriteIndex(CacheFile, Index, ErrorMsg, MsgSize)) {
			FFMS_DestroyFrameIndex(Index);
			Env->ThrowError("FFIndex: %s", ErrorMsg);
		}
		FFMS_DestroyFrameIndex(Index);
		if (!OverWrite)
			return AVSValue(1);
		else
			return AVSValue(2);
	} else {
		FFMS_DestroyFrameIndex(Index);
		return AVSValue(0);
	}
}

AVSValue __cdecl CreateFFVideoSource(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	FFMS_Init();

	char ErrorMsg[1024];
	unsigned MsgSize = sizeof(ErrorMsg);

	if (!Args[0].Defined())
    	Env->ThrowError("FFVideoSource: No source specified");

	const char *Source = Args[0].AsString();
	int Track = Args[1].AsInt(-1);
	bool Cache = Args[2].AsBool(true);
	const char *CacheFile = Args[3].AsString("");
	const char *PP = Args[4].AsString("");
	int Threads = Args[5].AsInt(-1);
	const char *Timecodes = Args[6].AsString("");
	int SeekMode = Args[7].AsInt(1);

	if (Track <= -2)
		Env->ThrowError("FFVideoSource: No video track selected");

	if (SeekMode < -1 || SeekMode > 3)
		Env->ThrowError("FFVideoSource: Invalid seekmode selected");

	if (Threads <= 0) 
		Threads = GetNumberOfLogicalCPUs();
	if (Threads < 1)
		Env->ThrowError("FFVideoSource: Invalid thread count");

	if (!_stricmp(Source, Timecodes))
		Env->ThrowError("FFVideoSource: Timecodes will overwrite the source");

	std::string DefaultCache(Source);
	DefaultCache.append(".ffindex");
	if (!strcmp(CacheFile, ""))
		CacheFile = DefaultCache.c_str();

	FrameIndex *Index;
	if (Cache) {
		if (!(Index = FFMS_ReadIndex(CacheFile, ErrorMsg, MsgSize))) {
			if (!(Index = FFMS_MakeIndex(Source, 0, NULL, NULL, NULL, ErrorMsg, MsgSize)))
				Env->ThrowError("FFVideoSource: %s", ErrorMsg);

			if (Cache)
				if (FFMS_WriteIndex(CacheFile, Index, ErrorMsg, MsgSize)) {
					FFMS_DestroyFrameIndex(Index);
					Env->ThrowError("FFVideoSource: %s", ErrorMsg);
				}
		}
	}

	AvisynthVideoSource *Filter;

	try {
		Filter = new AvisynthVideoSource(Source, Track, Index, PP, Threads, SeekMode, Env, ErrorMsg, MsgSize);
	} catch (...) {
		FFMS_DestroyFrameIndex(Index);	
		throw;
	}

	if (strcmp(Timecodes, "")) {
		if (FFMS_WriteTimecodes(FFMS_GetTITrackIndex(Index, Filter->GetTrack(), ErrorMsg, MsgSize), Timecodes, ErrorMsg, MsgSize)) {
			FFMS_DestroyFrameIndex(Index);
			delete Filter;
			Env->ThrowError("FFVideoSource: %s", ErrorMsg);
		}
	}

	FFMS_DestroyFrameIndex(Index);
	return Filter;
}

AVSValue __cdecl CreateFFAudioSource(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	FFMS_Init();

	char ErrorMsg[1024];
	unsigned MsgSize = sizeof(ErrorMsg);

	if (!Args[0].Defined())
    	Env->ThrowError("FFAudioSource: No source specified");

	const char *Source = Args[0].AsString();
	int Track = Args[1].AsInt(-1);
	bool Cache = Args[2].AsBool(true);
	const char *CacheFile = Args[3].AsString("");

	if (Track <= -2)
		Env->ThrowError("FFAudioSource: No video track selected");

	std::string DefaultCache(Source);
	DefaultCache.append(".ffindex");
	if (!strcmp(CacheFile, ""))
		CacheFile = DefaultCache.c_str();

	FrameIndex *Index;
	if (Cache) {
		if (!(Index = FFMS_ReadIndex(CacheFile, ErrorMsg, MsgSize))) {
			if (!(Index = FFMS_MakeIndex(Source, -1, NULL, NULL, NULL, ErrorMsg, MsgSize)))
				Env->ThrowError("FFAudioSource: %s", ErrorMsg);

			if (Cache)
				if (FFMS_WriteIndex(CacheFile, Index, ErrorMsg, MsgSize)) {
					FFMS_DestroyFrameIndex(Index);
					Env->ThrowError("FFAudioSource: %s", ErrorMsg);
				}
		}
	}

	AvisynthAudioSource *Filter;

	try {
		Filter = new AvisynthAudioSource(Source, Track, Index, Env, ErrorMsg, MsgSize);
	} catch (...) {
		FFMS_DestroyFrameIndex(Index);	
		throw;
	}

	FFMS_DestroyFrameIndex(Index);
	return Filter;
}

AVSValue __cdecl CreateFFPP(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	return new FFPP(Args[0].AsClip(), Args[1].AsString(""), Env);
}

AVSValue __cdecl CreateSWScale(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	return new SWScale(Args[0].AsClip(), Args[1].AsInt(0), Args[2].AsInt(0), Args[3].AsString("BICUBIC"), Args[4].AsString(""), Env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* Env) {
    Env->AddFunction("FFIndex", "[source]s[cachefile]s[trackmask]i[audiofile]s[overwrite]b", CreateFFIndex, 0);
    Env->AddFunction("FFVideoSource", "[source]s[track]i[cache]b[cachefile]s[pp]s[threads]i[timecodes]s[seekmode]i", CreateFFVideoSource, 0);
    Env->AddFunction("FFAudioSource", "[source]s[track]i[cache]b[cachefile]s", CreateFFAudioSource, 0);
	Env->AddFunction("FFPP", "c[pp]s", CreateFFPP, 0);
	Env->AddFunction("SWScale", "c[width]i[height]i[resizer]s[colorspace]s", CreateSWScale, 0);

    return "FFmpegSource - The Second Coming";
}
