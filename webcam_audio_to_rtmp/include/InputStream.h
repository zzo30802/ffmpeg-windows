#ifndef _ENCODE_FACTORY_H_
#define _ENCODE_FACTORY_H_
#include <future>
#include <iostream>

#include "DataManager.h"

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

// extern "C" {
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libswresample/swresample.h>
// #include <libswscale/swscale.h>
// }

// #pragma comment(lib, "swscale.lib")
// #pragma comment(lib, "avcodec.lib")
// #pragma comment(lib, "avutil.lib")
// #pragma comment(lib, "swresample.lib")
// get number of cpu

static int GetCpuNum() {
#if defined WIN32 || defined _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return static_cast<int>(sysinfo.dwNumberOfProcessors);
#elif defined __linux__
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
  int numCPU = 0;
  int mib[4];
  size_t len = sizeof(numCPU);

  // set the mib for hw.ncpu
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

  // get the number of CPUs from the system
  sysctl(mib, 2, &numCPU, &len, NULL, 0);

  if (numCPU < 1) {
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1)
      numCPU = 1;
  }
  return (int)numCPU;
#else
  return 1;  // default 1
#endif
}

class VideoInput {
 public:
  static VideoInput *Get(VideoStreamingContext *video_sc);
  VideoInput(VideoStreamingContext *video_sc);
  ~VideoInput();
  VideoInput() = delete;
  bool InitContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format);
  bool InitAndOpenAVCodecContext(enum AVCodecID video_codec);
  bool InitAndGetAVFrameFromData();
  // according to InitSwsContext passing arguments -> ex: AV_PIX_FMT_BGR24 -> AV_PIX_FMT_YUV420P
  Data ConvertPixelFromat(Data src_data);
  Data EncodeVideo(Data src_data);

 private:
  int &&dst_video_format = 0;
  VideoStreamingContext *sc;
  bool InitAVCodec(enum AVCodecID video_codec);  // use this codec to transfer AVPacket to ABFrame and output video
  bool InitAVCodecContext();
};

class AudioInput {
 public:
  static AudioInput *Get(AudioStreamingContext *audio_sc);
  AudioInput(AudioStreamingContext *audio_sc);
  ~AudioInput();
  AudioInput() = delete;
  bool InitContext(enum AVSampleFormat output_format, enum AVSampleFormat input_format);  // SwrContext
  bool InitAndOpenAVCodecContext(enum AVCodecID av_codec_id);
  bool InitAndGetAVFrameFromData();
  Data Resample(Data src_data);
  Data EncodeAudio(Data src_data);

 private:
  int &&output_sample_format = 0;
  int &&input_sample_format = 0;
  AudioStreamingContext *sc;
};

#endif  // _ENCODE_FACTORY_H_