@set DFSTRACINGON=FALSE
@set DXSDK_DIR=C:\Program Files (x86)\Microsoft DirectX SDK (March 2009)\
@set FP_NO_HOST_CHECK=NO
@set Framework35Version=v3.5
@set FrameworkDir=C:\Windows\Microsoft.NET\Framework64
@set FrameworkVersion=v2.0.50727
@set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\INCLUDE;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\INCLUDE;C:\Program Files\Microsoft SDKs\Windows\v6.0A\include;
@set LIB=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\LIB\amd64;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB\amd64;C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib\x64;
@set LIBPATH=C:\Windows\Microsoft.NET\Framework64\v3.5;C:\Windows\Microsoft.NET\Framework64\v2.0.50727;C:\Windows\Microsoft.NET\Framework64\v3.5;C:\Windows\Microsoft.NET\Framework64\v2.0.50727;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ATLMFC\LIB\amd64;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB\amd64;
@set LOCALAPPDATA=C:\Users\Aegisub\AppData\Local
@set OS=Windows_NT
@set Path=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN\amd64;C:\Windows\Microsoft.NET\Framework64\v3.5;C:\Windows\Microsoft.NET\Framework64\v3.5\Microsoft .NET Framework 3.5 (Pre-Release Version);C:\Windows\Microsoft.NET\Framework64\v2.0.50727;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\VCPackages;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\bin;C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin\x64;C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin\win64\x64;C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin;C:\Program Files (x86)\CollabNet Subversion Server;C:\cygwin\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Program Files\TortoiseSVN\bin;C:\Python25;C:\Python25\Scripts
@set PATHEXT=.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC;.PY
@set PROCESSOR_ARCHITECTURE=AMD64
@set PROCESSOR_IDENTIFIER=Intel64 Family 6 Model 15 Stepping 7, GenuineIntel
@set PROCESSOR_LEVEL=6
@set PROCESSOR_REVISION=0f07
@set TRACE_FORMAT_SEARCH_PATH=\\NTREL202.ntdev.corp.microsoft.com\34FB5F65-FFEB-4B61-BF0E-A6A76C450FAA\TraceFormat
@set VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC
@set VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\
@set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0
@set WindowsSdkDir=C:\Program Files\Microsoft SDKs\Windows\v6.0A\
@set TEMP=C:\Users\Aegisub\AppData\Local\Temp
@set TMP=C:\Users\Aegisub\AppData\Local\Temp
@CommonProgramFiles=C:\Program Files\Common Files
@CommonProgramFiles(x86)=C:\Program Files (x86)\Common Files
@ProgramFiles=C:\Program Files
@ProgramFiles(x86)=C:\Program Files (x86)


msbuild aegisub\tinderbox\windows\aegisub_vs2008.sln /p:Configuration=%1 /p:Platform=%2 /v:d /nologo /t:rebuild
