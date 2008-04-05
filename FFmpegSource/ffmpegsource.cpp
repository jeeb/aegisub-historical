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

#include "ffmpegsource.h"

int FFmpegSource::GetTrackIndex(int Index, CodecType ATrackType, IScriptEnvironment *Env) {
	if (Index == -1)
		for (unsigned int i = 0; i < FormatContext->nb_streams; i++)
			if (FormatContext->streams[i]->codec->codec_type == ATrackType) {
				Index = i;
				break;
			}

	if (Index == -1)
		Env->ThrowError("FFmpegSource: No %s track found", (ATrackType == CODEC_TYPE_VIDEO) ? "video" : "audio");
	if (Index <= -2)
		return -2;

	if (Index >= (int)FormatContext->nb_streams)
		Env->ThrowError("FFmpegSource: Invalid %s track number", (ATrackType ==  CODEC_TYPE_VIDEO) ? "video" : "audio");

	if (FormatContext->streams[Index]->codec->codec_type != ATrackType)
		Env->ThrowError("FFmpegSource: Selected track is not %s", (ATrackType == CODEC_TYPE_VIDEO) ? "video" : "audio");

	return Index;
}



FFmpegSource::FFmpegSource(const char *ASource, int AVideoTrack, int AAudioTrack, const char *ATimecodes,
	bool AVCache, const char *AVideoCache, const char *AAudioCache, const char *APPString,
	int AQuality, int AThreads, int ASeekMode, IScriptEnvironment *Env, FrameInfoVector *AFrames) {

	AFrames = &Frames;
	CurrentFrame = 0;
	SeekMode = ASeekMode;

	AVCodecContext *AudioCodecContext = NULL;
	AVCodec *AudioCodec;
	AVCodec *VideoCodec;

	FormatContext = NULL;
	VideoCodecContext = NULL;
	VideoCodec = NULL;

	if (av_open_input_file(&FormatContext, ASource, NULL, 0, NULL) != 0)
		Env->ThrowError("FFmpegSource: Couldn't open '%s'", ASource);
	
	if (av_find_stream_info(FormatContext) < 0)
		Env->ThrowError("FFmpegSource: Couldn't find stream information");

	VideoTrack = GetTrackIndex(AVideoTrack, CODEC_TYPE_VIDEO, Env);
	int AudioTrack = GetTrackIndex(AAudioTrack, CODEC_TYPE_AUDIO, Env);

	bool VCacheIsValid = true;
	bool ACacheIsValid = true;

	if (VideoTrack >= 0) {
		if (SeekMode >= 0 && av_seek_frame(FormatContext, VideoTrack, 0, AVSEEK_FLAG_BACKWARD) < 0)
			Env->ThrowError("FFmpegSource: Video track is unseekable");

		VCacheIsValid = LoadFrameInfoFromFile(AVideoCache, ASource, VideoTrack);		

		VideoCodecContext = FormatContext->streams[VideoTrack]->codec;
		VideoCodecContext->thread_count = AThreads;

		VideoCodec = avcodec_find_decoder(VideoCodecContext->codec_id);
		if (VideoCodec == NULL)
			Env->ThrowError("FFmpegSource: Video codec not found");

		if (avcodec_open(VideoCodecContext, VideoCodec) < 0)
			Env->ThrowError("FFmpegSource: Could not open video codec");

		// Fix for mpeg2 and other formats where decoding a frame is necessary to get information about the stream
		if (SeekMode >= 0 && (VideoCodecContext->pix_fmt == PIX_FMT_NONE || VideoCodecContext->width == 0 || VideoCodecContext->height == 0)) {
			int64_t Dummy;
			DecodeNextFrame(DecodeFrame, &Dummy);
			av_seek_frame(FormatContext, VideoTrack, 0, AVSEEK_FLAG_BACKWARD);
		}

		VI.image_type = VideoInfo::IT_TFF;
		VI.width = VideoCodecContext->width;
		VI.height = VideoCodecContext->height;
		VI.fps_denominator = FormatContext->streams[VideoTrack]->time_base.num;
		VI.fps_numerator = FormatContext->streams[VideoTrack]->time_base.den;

		if (VI.width <= 0 || VI.height <= 0)
			Env->ThrowError("FFmpegSource: Codec returned zero size video");

		// sanity check framerate
		if (VI.fps_denominator > VI.fps_numerator || VI.fps_denominator <= 0 || VI.fps_numerator <= 0) {
			VI.fps_denominator = 1;
			VI.fps_numerator = 30;
		}
		
		SetOutputFormat(VideoCodecContext->pix_fmt, Env);
		InitPP(VI.width, VI.height, APPString, AQuality, VideoCodecContext->pix_fmt, Env);
	}

	if (AudioTrack >= 0) {
		AudioCodecContext = FormatContext->streams[AudioTrack]->codec;

		AudioCodec = avcodec_find_decoder(AudioCodecContext->codec_id);
		if (AudioCodec == NULL)
			Env->ThrowError("FFmpegSource: Audio codec not found");

		if (avcodec_open(AudioCodecContext, AudioCodec) < 0)
			Env->ThrowError("FFmpegSource: Could not open audio codec");

		switch (AudioCodecContext->sample_fmt) {
			case SAMPLE_FMT_U8: VI.sample_type = SAMPLE_INT8; break;
			case SAMPLE_FMT_S16: VI.sample_type = SAMPLE_INT16; break;
			case SAMPLE_FMT_S24: VI.sample_type = SAMPLE_INT24; break;
			case SAMPLE_FMT_S32: VI.sample_type = SAMPLE_INT32; break;
			case SAMPLE_FMT_FLT: VI.sample_type = SAMPLE_FLOAT; break;
			default:
				Env->ThrowError("FFmpegSource: Unsupported/unknown sample format");
		}

		VI.nchannels = AudioCodecContext->channels;
		VI.audio_samples_per_second = AudioCodecContext->sample_rate;

		ACacheIsValid = OpenAudioCache(AAudioCache, ASource, AudioTrack, Env);
	}

	if ((!ACacheIsValid || !VCacheIsValid) && SeekMode == -1)
		Env->ThrowError("FFmpegSource: Unusual indexing error, report on doom9");	

	// Needs to be indexed?
	if (!ACacheIsValid || !VCacheIsValid) {
		FILE *RawCache = NULL;
		if (!ACacheIsValid)
			AudioCacheType = acRaw;	

		switch (AudioCacheType) {
			case acRaw: RawCache = NewRawCacheWriter(AAudioCache, ASource, AudioTrack, Env); break;
		}

		AVPacket Packet;
		while (av_read_frame(FormatContext, &Packet) >= 0) {
			if (Packet.stream_index == VideoTrack && !VCacheIsValid) {
				Frames.push_back(FrameInfo(Packet.dts, (Packet.flags & PKT_FLAG_KEY) ? 1 : 0));
				VI.num_frames++;
			} else if (Packet.stream_index == AudioTrack && !ACacheIsValid) {
				int Size = Packet.size;
				uint8_t *Data = Packet.data;

				while (Size > 0) {
					int TempOutputBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
					int Ret = avcodec_decode_audio2(AudioCodecContext, (int16_t *)DecodingBuffer, &TempOutputBufSize, Data, Size);
					if (Ret < 0)
						Env->ThrowError("FFmpegSource: Audio decoding error");

					int DecodedSamples = (int)VI.AudioSamplesFromBytes(TempOutputBufSize);

					Size -= Ret;
					Data += Ret;
					VI.num_audio_samples += DecodedSamples;

					if (AudioCacheType == acRaw) {
						fwrite(DecodingBuffer, 1, TempOutputBufSize, RawCache);
					}
				}
			}

			av_free_packet(&Packet);
		}

		if (!ACacheIsValid) {
			switch (AudioCacheType) {
				case acRaw: CloseRawCacheWriter(RawCache); break;
			}

			ACacheIsValid = OpenAudioCache(AAudioCache, ASource, AudioTrack, Env);
			if (!ACacheIsValid)
				Env->ThrowError("FFmpegSource: Failed to open newly created audio cache for reading");
		}

		if (VideoTrack >= 0 && VI.num_frames == 0)
			Env->ThrowError("FFmpegSource: Video track contains no frames");

		if (AudioTrack >= 0 && VI.num_audio_samples == 0)
			Env->ThrowError("FFmpegSource: Audio track contains no samples");

		if (VideoTrack >= 0)
			av_seek_frame(FormatContext, VideoTrack, Frames.front().DTS, AVSEEK_FLAG_BACKWARD);

		if (AVCache && !VCacheIsValid)
			if (!SaveFrameInfoToFile(AVideoCache, ASource, VideoTrack))
				Env->ThrowError("FFmpegSource: Failed to write video cache info");
	}

	if (AudioTrack >= 0)
		avcodec_close(AudioCodecContext);

	if (VideoTrack >= 0) {
		if (!SaveTimecodesToFile(ATimecodes, FormatContext->streams[VideoTrack]->time_base.num * 1000, FormatContext->streams[VideoTrack]->time_base.den))
			Env->ThrowError("FFmpegSource: Failed to write timecodes");

		// Adjust framerate to match the duration of the first frame
		if (Frames.size() >= 2) {
			unsigned int DTSDiff = (unsigned int)FFMAX(Frames[1].DTS - Frames[0].DTS, 1);
			VI.fps_denominator *= DTSDiff;
		}

		// Set AR variables
		Env->SetVar("FFSAR_NUM", VideoCodecContext->sample_aspect_ratio.num);
		Env->SetVar("FFSAR_DEN", VideoCodecContext->sample_aspect_ratio.den);
		Env->SetVar("FFSAR", av_q2d(VideoCodecContext->sample_aspect_ratio));
	}
}

FFmpegSource::~FFmpegSource() {
	if (VideoTrack >= 0)
		avcodec_close(VideoCodecContext);
	av_close_input_file(FormatContext);
}


int FFmpegSource::DecodeNextFrame(AVFrame *AFrame, int64_t *AStartTime) {
	AVPacket Packet;
	int FrameFinished = 0;
	int Ret = -1;
	*AStartTime = -1;

	while (av_read_frame(FormatContext, &Packet) >= 0) {
        if (Packet.stream_index == VideoTrack) {
			if (*AStartTime < 0)
				*AStartTime = Packet.dts;

			Ret = avcodec_decode_video(VideoCodecContext, AFrame, &FrameFinished, Packet.data, Packet.size);
        }

        av_free_packet(&Packet);

		if (FrameFinished)
			goto Done;
	}

	// Flush the last frames
	if (VideoCodecContext->has_b_frames)
		Ret = avcodec_decode_video(VideoCodecContext, AFrame, &FrameFinished, NULL, 0);

	if (!FrameFinished)
		goto Error;

// Ignore errors for now
Error:
Done:
	return Ret;
}

PVideoFrame FFmpegSource::GetFrame(int n, IScriptEnvironment* Env) {
	if (LastFrameNum == n)
		return LastFrame;

	bool HasSeeked = false;

	if (SeekMode >= 0) {
		int ClosestKF = FindClosestKeyFrame(n);

		if (SeekMode == 0) {
			if (n < CurrentFrame) {
				av_seek_frame(FormatContext, VideoTrack, 0, AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(VideoCodecContext);
				CurrentFrame = 0;
			}
		} else {
			// 10 frames is used as a margin to prevent excessive seeking since the predicted best keyframe isn't always selected by avformat
			if (n < CurrentFrame || ClosestKF > CurrentFrame + 10 || (SeekMode == 3 && n > CurrentFrame + 10)) {
				av_seek_frame(FormatContext, VideoTrack, (SeekMode == 3) ? Frames[n].DTS : Frames[ClosestKF].DTS, AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(VideoCodecContext);
				HasSeeked = true;
			}
		}
	} else if (n < CurrentFrame) {
		Env->ThrowError("FFmpegSource: Non-linear access attempted");	
	}

	do {
		int64_t StartTime;
		DecodeNextFrame(DecodeFrame, &StartTime);

		if (HasSeeked) {
			HasSeeked = false;

			// Is the seek destination time known? Does it belong to a frame?
			if (StartTime < 0 || (CurrentFrame = FrameFromDTS(StartTime)) < 0) {
				switch (SeekMode) {
					case 1:
						Env->ThrowError("FFmpegSource: Frame accurate seeking is not possible in this file");
					case 2:
					case 3:
						CurrentFrame = ClosestFrameFromDTS(StartTime);
						break;
					default:
						Env->ThrowError("FFmpegSource: Failed assertion");
				}	
			}
		}

		CurrentFrame++;
	} while (CurrentFrame <= n);

	LastFrame = OutputFrame(DecodeFrame, Env);
	LastFrameNum = n;
	return LastFrame;
}
