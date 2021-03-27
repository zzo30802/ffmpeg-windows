#ifndef _ENCODE_FACTORY_H_
#define _ENCODE_FACTORY_H_
#include <future>
#include <iostream>

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

typedef struct VideoStreamingContext {
  int img_width = 1280;
  int img_height = 720;
  int pixel_size = 3;
  // int fps = 25;
  SwsContext *sws_context = nullptr;
  AVFormatContext *av_format_context = nullptr;
  AVCodec *av_codec = nullptr;
  AVStream *av_stream = nullptr;
  AVCodecContext *av_codec_context = nullptr;
  AVFrame *yuv = nullptr;
  AVPacket vpack = {0};
  int index;
  char *outUrl;
} VideoStreamingContext;

typedef struct AudioStreamingContext {
  int channels = 2;
  int sample_rate = 44100;
  int sample_byte = 16;
  int nb_samples = 1024;
  SwsContext *sws_context = nullptr;
  AVFormatContext *format_context = nullptr;
  AVCodec *av_codec = nullptr;
  AVStream *av_stream = nullptr;
  AVCodecContext *av_codec_context = nullptr;
  AVFrame *pcm = nullptr;
  AVPacket apack = {0};
  int index;
  char *outUrl;
} AudioStreamingContext;

static bool ErrorMesssage(const int &error_num) {
  char buf[1024]{0};
  // put error code to buf
  av_strerror(error_num, buf, sizeof(buf));
  std::cout << buf << std::endl;
  return false;
}

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

// abstract class
class EncodeFacotry {
  /*
  process flow :
  input -> SwsContext -> AVFormatContext -> AVStream -> (encode) -> AVPacket
  -> AVCodec(decode) -> AVFrame -> output 
  */
 public:
  // SwsContext
  virtual bool InitSwsContext() = 0;
  // AVFormatContext
  virtual bool InitAVFormatContext() = 0;
  virtual bool InitAVCodec() = 0;
  virtual bool InitAVCodecContext() = 0;
  // initialize AVStream, AVCodec, AVCodecContext
  //**************Encoding***************

  //**************Decoding***************
};

class VideoProcess : public EncodeFacotry {
 public:
  VideoProcess(VideoStreamingContext *video_sc);
  ~VideoProcess();
  VideoProcess() = delete;
  bool InitSwsContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format);
  bool InitAVFormatContext();
  bool InitAVCodec(AVCodecID video_codec);  // use this codec to transfer AVPacket to ABFrame and output video
  bool InitAVCodecContext();

 private:
  VideoStreamingContext *sc;
};

class AudioProcess : public EncodeFacotry {
 public:
  AudioProcess(AudioStreamingContext *audio_sc);
  ~AudioProcess();
  AudioProcess() = delete;
  bool InitSwsContext();
  bool InitAVFormatContext();
  bool InitAVCodec();
  bool InitAVCodecContext();

 private:
  AudioStreamingContext *sc;
};

#endif  // _ENCODE_FACTORY_H_