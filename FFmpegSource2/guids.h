/*
 * Copyright (c) 2004-2008 Mike Matsnev.  All Rights Reserved.
 * 
 * $Id: guids.h,v 1.16 2008/03/29 15:41:30 mike Exp $
 * 
 */

#ifndef GUIDS_H
#define	GUIDS_H

// {941A4793-A705-4312-8DFC-C11CA05F397E}
DEFINE_GUID(CLSID_RealAudioDecoder,
0x941A4793, 0xA705, 0x4312, 0x8D, 0xFC, 0xC1, 0x1C, 0xA0, 0x5F, 0x39, 0x7E);

// {B8CBBAD8-AA1A-4cea-B95E-730041A55EF0}
DEFINE_GUID(MEDIASUBTYPE_WavpackHybrid, 
0xb8cbbad8, 0xaa1a, 0x4cea, 0xb9, 0x5e, 0x73, 0x0, 0x41, 0xa5, 0x5e, 0xf0);

// {1541C5C0-CDDF-477d-BC0A-86F8AE7F8354}
DEFINE_GUID(MEDIASUBTYPE_FLAC_FRAMED,
0x1541c5c0, 0xcddf, 0x477d, 0xbc, 0xa, 0x86, 0xf8, 0xae, 0x7f, 0x83, 0x54);

// {8D2FD10B-5841-4a6b-8905-588FEC1ADED9}
DEFINE_GUID(MEDIASUBTYPE_Vorbis2,
0x8d2fd10b, 0x5841, 0x4a6b, 0x89, 0x5, 0x58, 0x8f, 0xec, 0x1a, 0xde, 0xd9);

// {B36E107F-A938-4387-93C7-55E966757473}
DEFINE_GUID(FORMAT_VorbisFormat2,
0xb36e107f, 0xa938, 0x4387, 0x93, 0xc7, 0x55, 0xe9, 0x66, 0x75, 0x74, 0x73);

// {E487EB08-6B26-4be9-9DD3-993434D313FD}
DEFINE_GUID(MEDIATYPE_Subtitle,
0xe487eb08, 0x6b26, 0x4be9, 0x9d, 0xd3, 0x99, 0x34, 0x34, 0xd3, 0x13, 0xfd);

// {87C0B230-03A8-4fdf-8010-B27A5848200D}
DEFINE_GUID(MEDIASUBTYPE_UTF8,
0x87c0b230, 0x3a8, 0x4fdf, 0x80, 0x10, 0xb2, 0x7a, 0x58, 0x48, 0x20, 0xd);

// {3020560F-255A-4ddc-806E-6C5CC6DCD70A}
DEFINE_GUID(MEDIASUBTYPE_SSA,
0x3020560f, 0x255a, 0x4ddc, 0x80, 0x6e, 0x6c, 0x5c, 0xc6, 0xdc, 0xd7, 0xa);

// {326444F7-686F-47ff-A4B2-C8C96307B4C2}
DEFINE_GUID(MEDIASUBTYPE_ASS,
0x326444f7, 0x686f, 0x47ff, 0xa4, 0xb2, 0xc8, 0xc9, 0x63, 0x7, 0xb4, 0xc2);

// {370689E7-B226-4f67-978D-F10BC1A9C6AE}
DEFINE_GUID(MEDIASUBTYPE_ASS2, 
0x370689e7, 0xb226, 0x4f67, 0x97, 0x8d, 0xf1, 0xb, 0xc1, 0xa9, 0xc6, 0xae);

// {B753B29A-0A96-45be-985F-68351D9CAB90}
DEFINE_GUID(MEDIASUBTYPE_USF,
0xb753b29a, 0xa96, 0x45be, 0x98, 0x5f, 0x68, 0x35, 0x1d, 0x9c, 0xab, 0x90);

// {F7239E31-9599-4e43-8DD5-FBAF75CF37F1}
DEFINE_GUID(MEDIASUBTYPE_VOBSUB,
0xf7239e31, 0x9599, 0x4e43, 0x8d, 0xd5, 0xfb, 0xaf, 0x75, 0xcf, 0x37, 0xf1);

// {0B10E53F-ABF9-4581-BE9C-2C9A5EC6F2E0}
DEFINE_GUID(MEDIASUBTYPE_VOBSUB2,
0xb10e53f, 0xabf9, 0x4581, 0xbe, 0x9c, 0x2c, 0x9a, 0x5e, 0xc6, 0xf2, 0xe0);

// {5965E924-63F9-4a64-B71E-F75188FD6384}
DEFINE_GUID(MEDIASUBTYPE_DXRSub,
0x5965e924, 0x63f9, 0x4a64, 0xb7, 0x1e, 0xf7, 0x51, 0x88, 0xfd, 0x63, 0x84);

// {A33D2F7D-96BC-4337-B23B-A8B9FBC295E9}
DEFINE_GUID(FORMAT_SubtitleInfo,
0xa33d2f7d, 0x96bc, 0x4337, 0xb2, 0x3b, 0xa8, 0xb9, 0xfb, 0xc2, 0x95, 0xe9);

DEFINE_GUID(MEDIASUBTYPE_AC3,
0x00002000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_EAC3,
0x0000EAC3, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_DTS,
0x00002001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// {B855F837-194F-4d9a-9358-E31ED6B49D03}
DEFINE_GUID(MEDIASUBTYPE_VC1,
0xb855f837, 0x194f, 0x4d9a, 0x93, 0x58, 0xe3, 0x1e, 0xd6, 0xb4, 0x9d, 0x3);

// {93A22E7A-5091-45ef-BA61-6DA26156A5D0}
DEFINE_GUID(CLSID_DirectVobSubFilter, 
0x93a22e7a, 0x5091, 0x45ef, 0xba, 0x61, 0x6d, 0xa2, 0x61, 0x56, 0xa5, 0xd0);

// {9852A670-F845-491b-9BE6-EBD841B8A613}
DEFINE_GUID(CLSID_DirectVobSubFilter2, 
0x9852a670, 0xf845, 0x491b, 0x9b, 0xe6, 0xeb, 0xd8, 0x41, 0xb8, 0xa6, 0x13);

// {F6D90F11-9C73-11D3-B32E-00C04F990BB4}
DEFINE_GUID(CLSID_MSXML2,
0xF6D90F11, 0x9C73, 0x11D3, 0xB3, 0x2E, 0x00, 0xC0, 0x4F, 0x99, 0x0B, 0xB4);

// {9F062738-CD84-4F54-A3C4-BD5EB44F416B}
DEFINE_GUID(CLSID_SonicAudioDec,
0x9F062738, 0xCD84, 0x4F54, 0xA3, 0xC4, 0xBD, 0x5E, 0xB4, 0x4F, 0x41, 0x6B);

// {53D9DE0B-FC61-4650-9773-74D13CC7E582}
DEFINE_GUID(CLSID_DiskFile,
0x53d9de0b, 0xfc61, 0x4650, 0x97, 0x73, 0x74, 0xd1, 0x3c, 0xc7, 0xe5, 0x82);

// {BD4FB4BE-809D-487b-ADD6-F7D164247E52}
DEFINE_GUID(CLSID_HTTPStream,
0xbd4fb4be, 0x809d, 0x487b, 0xad, 0xd6, 0xf7, 0xd1, 0x64, 0x24, 0x7e, 0x52);

// {90C7D10E-CE9A-479b-A238-1A0F2396DE43}
DEFINE_GUID(CLSID_MemAlloc,
0x90c7d10e, 0xce9a, 0x479b, 0xa2, 0x38, 0x1a, 0xf, 0x23, 0x96, 0xde, 0x43);

DEFINE_GUID(MEDIASUBTYPE_WVC1,
0x31435657, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_WMV3,
0x33564d57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// FIXME: move somewhere else?
DEFINE_GUID(Haali_TS_Parser,
0xB841F346, 0x4835, 0x4de8, 0xAA, 0x5E, 0x2E, 0x7C, 0xD2, 0xD4, 0xC4, 0x35);

typedef struct tagVORBISFORMAT2
{
	DWORD Channels;
	DWORD SamplesPerSec;
	DWORD BitsPerSample;	
	DWORD HeaderSize[3]; // 0: Identification, 1: Comment, 2: Setup
} VORBISFORMAT2, *PVORBISFORMAT2, FAR *LPVORBISFORMAT2;

#pragma pack(push, 1)
typedef struct {
	DWORD dwOffset;	
	CHAR IsoLang[4]; // three letter lang code + terminating zero
	WCHAR TrackName[256]; // 256 bytes ought to be enough for everyone :)
} SUBTITLEINFO;
#pragma pack(pop)

#endif