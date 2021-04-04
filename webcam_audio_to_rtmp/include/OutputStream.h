#ifndef _OUTPUT_FACTORY_H_
#define _OUTPUT_FACTORY_H_

#include "DataManager.h"

// class AVCodecContext;

class OutputStream {
 public:
  OutputStream();
  ~OutputStream();  // Release
  // Singleton
  static OutputStream *Get();
  bool InitOutputAVFormatContext(const char *url);
  int InitAVCodecContextAndAVStream(const AVCodecContext *av_codec_context);
  bool OpenOutputURL();
  bool AddAVStreamToAVFormatContext(Data src_data, const int &av_stream_index);

 private:
  static OutputStream *o_instance;
  AVFormatContext *output_av_format_context = nullptr;
  const AVCodecContext *output_video_av_codec_context = nullptr;
  const AVCodecContext *output_audio_av_codec_context = nullptr;
  AVStream *output_video_stream = nullptr;
  AVStream *output_audio_stream = nullptr;
  const char *url;
};

#endif  // _OUTPUT_FACTORY_H_