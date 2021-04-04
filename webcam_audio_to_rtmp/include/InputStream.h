#ifndef _INPUT_STREAM_H_
#define _INPUT_STREAM_H_
#include "DataManager.h"

class AVCodecContext;

class VideoInputFactory {
 public:
  static VideoInputFactory *Get(unsigned char index = 0);
  VideoInputFactory(){};
  virtual ~VideoInputFactory(){};
  virtual bool InitContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format) = 0;
  virtual bool InitAndOpenAVCodecContext(enum AVCodecID video_codec) = 0;
  virtual bool InitAVFrame() = 0;
  virtual Data ConvertPixelFormat(Data frame) = 0;
  virtual Data EncodeVideo(Data frame) = 0;
  // provide the param to other class funtion to use
  AVCodecContext *av_codec_context = nullptr;

 public:
  int src_img_width = 1280;
  int src_img_height = 720;
  int dst_img_width = 1280;
  int dst_img_height = 720;
  const int &&pixel_size = 3;
  int fps = 25;

 protected:
  //*******enum*******
  int src_pixel_format = 0;
  int dst_pixel_format = 0;
  int codec_id = 0;
};

class AudioInputFactory {
 public:
  static AudioInputFactory *Get(unsigned char index = 0);
  AudioInputFactory(){};
  virtual ~AudioInputFactory(){};
  virtual bool InitContext(enum AVSampleFormat output_format, enum AVSampleFormat input_format) = 0;
  virtual bool InitAndOpenAVCodecContext(enum AVCodecID av_codec_id) = 0;
  virtual bool InitAVFrame() = 0;
  virtual Data Resample(Data src_data) = 0;
  virtual Data EncodeAudio(Data src_data) = 0;
  // provide the param to other class funtion to use
  AVCodecContext *av_codec_context = nullptr;

 public:
  int channels = 2;
  int sample_rate = 44100;
  int sample_byte = 2;
  int nb_sample = 1024;
  long long last_pts = -1;

 protected:
  //*******enum*******
  int src_sample_format = 0;
  int dst_sample_format = 0;
  int codec_id = 0;
};
#endif  // _INPUT_STREAM_H_