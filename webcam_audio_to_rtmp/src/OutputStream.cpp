#include "OutputStream.h"

#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
// #pragma comment(lib, "swscale.lib")
// #pragma comment(lib, "avcodec.lib")
// #pragma comment(lib, "avutil.lib")
// #pragma comment(lib, "swresample.lib")

OutputStream* OutputStream::o_instance = nullptr;

OutputStream::OutputStream() {
  avformat_network_init();
}

OutputStream* OutputStream::Get() {
  if (o_instance == nullptr)
    o_instance = new OutputStream();
  return o_instance;
}

bool OutputStream::InitOutputAVFormatContext(const char* url) {
  int ret = avformat_alloc_output_context2(&this->output_av_format_context, 0, "flv", url);
  this->url = url;
  if (ret != 0) {
    std::cout << "Error: VideoProcess::InitAVFormatContext()" << std::endl;
    return ErrorMesssage(ret);
  }
  // this->url = url;
  return true;
}

// if fail will return -1

int OutputStream::InitAVCodecContextAndAVStream(const AVCodecContext* av_codec_context) {
  if (!av_codec_context)
    return -1;
  // create a new stream in AVFormatContext
  AVStream* st = avformat_new_stream(this->output_av_format_context, nullptr);
  if (!st) {
    std::cout << "Error: OutputStream::AddAVStreamToOutputAVFormatContext()" << std::endl;
    std::cout << "  ->avformat_new_stream()" << std::endl;
    return -1;
  }
  st->codecpar->codec_tag = 0;
  // copy parameter from codec
  avcodec_parameters_from_context(st->codecpar, av_codec_context);
  av_dump_format(this->output_av_format_context, 0, this->url, 1);

  // video or audio
  if (av_codec_context->codec_type == AVMEDIA_TYPE_VIDEO) {
    this->output_video_av_codec_context = av_codec_context;
    this->output_video_stream = st;
  } else if (av_codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
    this->output_audio_av_codec_context = av_codec_context;
    this->output_audio_stream = st;
  }
  return st->index;
}

bool OutputStream::OpenOutputURL() {
  int ret = avio_open(&this->output_av_format_context->pb, this->url, AVIO_FLAG_WRITE);
  if (ret != 0) {
    std::cout << "Error: OutputStream::OpenOutputURL()" << std::endl;
    std::cout << "  -> avio_open()" << std::endl;
    return ErrorMesssage(ret);
  }
  ret = avformat_write_header(this->output_av_format_context, nullptr);
  if (ret != 0) {
    std::cout << "Error: OutputStream::OpenOutputURL()" << std::endl;
    std::cout << "  -> avformat_write_header()" << std::endl;
    return ErrorMesssage(ret);
  }
  return true;
}

bool OutputStream::AddAVStreamToAVFormatContext(Data src_data, const int& av_stream_index) {
  // if no data input
  if (!src_data.data || src_data.size <= 0) {
    std::cout << "Error: OutputStream::AddAVStreamToAVFormatContext()" << std::endl;
    std::cout << "  -> (Data) src_data.data = nullptr || src_data.size <=0" << std::endl;
    return false;
  }
  AVPacket* pack = (AVPacket*)src_data.data;
  pack->stream_index = av_stream_index;
  AVRational stime;
  AVRational dtime;

  if (this->output_video_stream && this->output_video_av_codec_context &&
      pack->stream_index == this->output_video_stream->index) {
    stime = this->output_video_av_codec_context->time_base;
    dtime = this->output_video_stream->time_base;
  } else if (this->output_audio_stream && this->output_audio_av_codec_context &&
             pack->stream_index == this->output_audio_stream->index) {
    stime = this->output_audio_av_codec_context->time_base;
    dtime = this->output_audio_stream->time_base;
  } else {
    std::cout << "Error: OutputStream::AddAVStreamToAVFormatContext()" << std::endl;
    return false;
  }

  // upstreaming
  /*
  int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq) av_const;
  a  : initinal value
  bq : original time_base
  cq : The value you want to switch
  */
  pack->pts = av_rescale_q(pack->pts, stime, dtime);
  pack->dts = av_rescale_q(pack->dts, stime, dtime);
  // std::cout << "pack->pts : " << pack->pts << std::endl;
  // std::cout << "pack->dts : " << pack->dts << std::endl;
  pack->duration = av_rescale_q(pack->duration, stime, dtime);

  int ret = av_interleaved_write_frame(this->output_av_format_context, pack);

  if (ret != 0) {
    std::cout << "Error: OutputStream::AddAVStreamToAVFormatContext()" << std::endl;
    std::cout << "  -> av_interleaved_write_frame()" << std::endl;
    return false;
  }
  std::cout << "#" << std::flush;
  return true;
}

OutputStream::~OutputStream() {
  std::cout << "OutputStream instance release start" << std::endl;
  if (this->output_av_format_context) {
    avformat_close_input(&this->output_av_format_context);
    this->output_video_stream = nullptr;
  }
  this->output_video_av_codec_context = nullptr;
  this->url = "";
}