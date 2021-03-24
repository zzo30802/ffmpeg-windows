git clean -fdx
cmake ../ -G "Visual Studio 15 2017" -A x64
cmake --build . --config Release

ECHO %THIRD_PARTY%
rem opencv
COPY  %THIRD_PARTY%\opencv\build\x64\vc15\bin\opencv_world349.dll  %~dp0\bin\Release
rem ffmpeg
COPY  %THIRD_PARTY%\ffmpeg\bin\avcodec-58.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\avdevice-58.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\avfilter-7.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\avformat-58.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\avresample-4.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\avutil-56.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\postproc-55.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\swresample-3.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\swscale-5.dll  %~dp0\bin\Release
COPY  %THIRD_PARTY%\ffmpeg\bin\libx264.dll  %~dp0\bin\Release
rem Qt
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Cored.dll %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Cored.pdb %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Guid.dll %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Guid.pdb %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Multimediad.dll %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Multimediad.pdb %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Networkd.dll %~dp0\bin\Release
COPY C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\bin\Qt5Networkd.pdb %~dp0\bin\Release
rem Qt audio
mkdir %~dp0\bin\Release\audio
xcopy C:\Qt\Qt5_14_2\5.14.2\msvc2017_64\plugins\audio %~dp0\bin\Release\audio