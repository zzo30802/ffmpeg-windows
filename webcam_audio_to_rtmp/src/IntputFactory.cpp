#include "InputFactory.h"
/***************************************************
 *                      Video                      *
 ***************************************************/
//-----public-----
VideoInput::VideoInput(VideoStreamingContext *video_sc) {
  this->sc = video_sc;
}
bool VideoInput::InitSwsContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format) {
  // init SwsContext
  this->sc->sws_context = sws_getCachedContext(this->sc->sws_context,
                                               this->sc->img_width, this->sc->img_height, src_format,
                                               this->sc->img_width, this->sc->img_height, dst_format,
                                               SWS_BICUBIC, nullptr, nullptr, nullptr);
  this->dst_video_format = dst_format;

  if (!this->sc->sws_context) {
    std::cout << "ERROR: VideoProcess::InitSwsContext()" << std::endl;
    std::cout << "  ->sws_getCachedContext() failed." << std::endl;
    return false;
  }
}

bool VideoInput::InitAndOpenAVCodecContext(enum AVCodecID video_codec, enum AVPixelFormat pixel_format) {
  // init AVCodec
  if (!this->InitAVCodec(video_codec))
    return false;
  // init AVCodecContext
  if (!this->InitAVCodecContext(pixel_format))
    return false;
  // copy AVCodec parameters to AVCodecContext
  std::future<int> fuRes = std::async(std::launch::async, avcodec_open2,
                                      this->sc->av_codec_context, nullptr, nullptr);
  int ret = fuRes.get();
  if (ret != 0) {
    std::cout << "Error: VideoProcess::InitAndOpenAVCodecContext()" << std::endl;
    char err[1024] = {0};
    av_strerror(ret, err, sizeof(err) - 1);
    std::cout << err << std::endl;
    avcodec_free_context(&this->sc->av_codec_context);
    return false;
  }
  return false;
}
bool VideoInput::InitAndGetAVFrameFromData() {
  // Init AVFrame
  this->sc->yuv = av_frame_alloc();
  this->sc->yuv->format = this->dst_video_format;
  this->sc->yuv->width = this->sc->img_width;
  this->sc->yuv->height = this->sc->img_height;
  this->sc->yuv->pts = 0;
  // allocate buffer
  int ret = av_frame_get_buffer(this->sc->yuv, 32);
  if (ret != 0) {
    std::cout << "Error: VideoInput::InitAndGetAVFrameFromData()" << std::endl;
    return ErrorMesssage(ret);
  }
  return true;
}

// according to InitSwsContext passing arguments -> ex: AV_PIX_FMT_BGR24 -> AV_PIX_FMT_YUV420P
Data VideoInput::ConvertPixelFromat(Data src_data) {
  Data tmp;
  tmp.pts = src_data.pts;  // copy pts
  uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
  indata[0] = (uint8_t *)src_data.data;  //****
  int insize[AV_NUM_DATA_POINTERS] = {0};

  insize[0] = this->sc->img_width * this->sc->pixel_size;
  int h = sws_scale(this->sc->sws_context, indata, insize, 0, this->sc->img_height,
                    this->sc->yuv->data, this->sc->yuv->linesize);
  if (h <= 0) {
    std::cout << "Error: VideoInput::ConvertPixelFromat()" << std::endl;
    std::cout << "  ->sws_scale() : the height of the output slice <= 0." << std::endl;
    return tmp;
  }
  this->sc->yuv->pts = src_data.pts;
  tmp.data = (char *)(this->sc->yuv);
  int *p = this->sc->yuv->linesize;
  while ((*p)) {
    tmp.size += (*p) * this->sc->img_height;
    p++;
  }
  return tmp;
}

Data VideoInput::EncodeVideo(Data src_data) {
  // clean AVPacktet and the initialize it(set data & size to 0)
  /*
  void av_packet_unref(AVPacket *pkt)
  {
    av_packet_free_side_data(pkt);
    av_buffer_unref(&pkt->buf);
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
  }
  */
  av_packet_unref(&this->sc->av_pack);
  Data tmp;
  if (src_data.size <= 0 || !src_data.data) {
    std::cout << "Error: VideoInput::EncodeVideo" << std::endl;
    std::cout << "  -> av_packet_unref() : Data src_data.size<=0 || src_data.data=nullptr" << std::endl;
    return tmp;
  }
  // convert Data to AVFrame
  AVFrame *p_av_frame = (AVFrame *)src_data.data;
  // passing data from AVFrame to AVCodecContext
  int ret = avcodec_send_frame(this->sc->av_codec_context, p_av_frame);
  if (ret != 0) {
    std::cout << "Error: VideoInput::EncodeVideo" << std::endl;
    return tmp;
  }
  // receive data from buffer(AVCodecContext). if return 0 : can receice data
  ret = avcodec_receive_packet(this->sc->av_codec_context, &this->sc->av_pack);
  if (ret != 0) {
    std::cout << "Error: VideoInput::EncodeVideo()" << std::endl;
    std::cout << "  -> avcodec_receive_packet()" << std::endl;
    return tmp;
  }
  tmp.data = (char *)&(this->sc->av_pack);  // copy
  tmp.size = this->sc->av_pack.size;        // copy
  tmp.pts = src_data.pts;                   // get time from src_data
  return tmp;
}

// private
bool VideoInput::InitAVCodec(enum AVCodecID video_codec) {
  this->sc->av_codec = avcodec_find_encoder(video_codec);
  if (!this->sc->av_codec) {
    std::cout << "Error: VideoProcess::InitAVCodec()" << std::endl;
    return false;
  }
  return true;
}
bool VideoInput::InitAVCodecContext(enum AVPixelFormat pixel_format) {
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
  this->sc->av_codec_context->time_base = {1, 1000000};

  this->sc->av_codec_context->bit_rate = 50 * 1024 * 8;
  this->sc->av_codec_context->width = this->sc->img_width;
  this->sc->av_codec_context->height = this->sc->img_height;
  this->sc->av_codec_context->framerate = {this->sc->fps, 1};
  this->sc->av_codec_context->gop_size = 50;
  this->sc->av_codec_context->max_b_frames = 0;
  this->sc->av_codec_context->pix_fmt = pixel_format;
  return true;
}

/***************************************************
 *                      Audio                      *
 ***************************************************/
AudioInput::AudioInput(AudioStreamingContext *audio_sc) {
  this->sc = audio_sc;
}
bool AudioInput::InitSwsContext() {
}
