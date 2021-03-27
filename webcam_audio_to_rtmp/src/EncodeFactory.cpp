#include "EncodeFactory.h"
/***************************************************
 *                      Video                      *
 ***************************************************/
// public
VideoProcess::VideoProcess(VideoStreamingContext *video_sc) {
  this->sc = video_sc;
}
bool VideoProcess::InitSwsContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format) {
  // init SwsContext
  this->sc->sws_context = sws_getCachedContext(this->sc->sws_context,
                                               this->sc->img_width, this->sc->img_height, src_format,
                                               this->sc->img_width, this->sc->img_height, dst_format,
                                               SWS_BICUBIC, nullptr, nullptr, nullptr);
  if (!sc->sws_context) {
    std::cout << "ERROR: VideoProcess::InitSwsContext()" << std::endl;
    std::cout << "  ->sws_getCachedContext() failed." << std::endl;
    return false;
  }
}

bool VideoProcess::InitAVFormatContext() {
  int ret = avformat_alloc_output_context2(&this->sc->av_format_context, nullptr, "flv", this->sc->outUrl);
  if (!ret) {
    std::cout << "Error: VideoProcess::InitAVFormatContext()" << std::endl;
    return ErrorMesssage(ret);
  }
  return true;
}

bool VideoProcess::InitAVCodec(AVCodecID video_codec) {
  this->sc->av_codec = avcodec_find_encoder(video_codec);
  if (!this->sc->av_codec) {
    std::cout << "Error: VideoProcess::InitAVCodec()" << std::endl;
    return false;
  }
  return true;
}

bool VideoProcess::InitAVCodecContext() {
  if (!this->sc->av_codec) {  // check
    std::cout << "Error: VideoProcess::InitAVCodecContext()" << std::endl;
    return false;
  }
  this->sc->av_codec_context = avcodec_alloc_context3(this->sc->av_codec);
  if (!this->sc->av_codec_context) {
    std::cout << "Error: VideoProcess::InitAVCodecContext()" << std::endl;
    std::cout << "  -> avcodec_alloc_context3()" << std::endl;
    return false;
  }
  this->sc->av_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  this->sc->av_codec_context->thread_count = GetCpuNum();
  this->sc->time_base = {1, 1000000};
  return true;
}

/***************************************************
 *                      Audio                      *
 ***************************************************/
AudioProcess::AudioProcess(AudioStreamingContext *audio_sc) {
  this->sc = audio_sc;
}
bool AudioProcess::InitSwsContext() {
}
bool AudioProcess::InitAVFormatContext() {
}
