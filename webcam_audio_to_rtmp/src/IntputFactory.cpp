#include "InputStream.h"
/***************************************************
 *                      Video                      *
 ***************************************************/
//-----public-----
VideoInput *VideoInput::Get(VideoStreamingContext *video_sc) {
  static VideoInput instance(video_sc);
  return &instance;  // return pointer
}
VideoInput::VideoInput(VideoStreamingContext *video_sc) {
  this->sc = video_sc;
}
bool VideoInput::InitContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format) {
  // init SwsContext
  this->sc->sws_context = sws_getCachedContext(this->sc->sws_context,
                                               this->sc->img_width, this->sc->img_height, AV_PIX_FMT_BGR24,
                                               this->sc->img_width, this->sc->img_height, AV_PIX_FMT_YUV420P,
                                               SWS_BICUBIC, 0, 0, 0);
  this->dst_video_format = dst_format;

  if (!this->sc->sws_context) {
    std::cout << "ERROR: VideoProcess::InitSwsContext()" << std::endl;
    std::cout << "  ->sws_getCachedContext() failed." << std::endl;
    return false;
  }
  return true;
}

bool VideoInput::InitAndOpenAVCodecContext(enum AVCodecID video_codec) {
  // init AVCodec
  if (!this->InitAVCodec(video_codec))
    return false;
  // init AVCodecContext
  if (!this->InitAVCodecContext())
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
  return true;
}
bool VideoInput::InitAndGetAVFrameFromData() {
  // Init AVFrame
  this->sc->yuv = av_frame_alloc();
  this->sc->yuv->format = AV_PIX_FMT_YUV420P;  //(AVPixelFormat)(this->dst_video_format);
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
  std::cout << "ConvertPixelFromat 1" << std::endl;
  std::cout << "src_data.size : " << src_data.size << std::endl;
  if (!this->sc->sws_context) {
    std::cout << "QQQQQQQQQQQQQQQQQQQQQQQQQ" << std::endl;
  }
  if (!this->sc->yuv) {
    std::cout << "TTTTTTTTTTTTTTTTTTTTTTTTT" << std::endl;
  }
  Data tmp;
  tmp.pts = src_data.pts;
  //Input data
  uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
  //indata[0] bgrbgrbgr
  //plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
  indata[0] = (uint8_t *)src_data.data;
  int insize[AV_NUM_DATA_POINTERS] = {0};

  insize[0] = this->sc->img_width * this->sc->pixel_size;
  std::cout << "ConvertPixelFromat 6" << std::endl;
  int h = sws_scale(this->sc->sws_context, indata, insize, 0, this->sc->img_height,
                    this->sc->yuv->data, this->sc->yuv->linesize);
  std::cout << "ConvertPixelFromat 7" << std::endl;
  if (h <= 0) {
    return tmp;
  }
  this->sc->yuv->pts = src_data.pts;
  tmp.data = (char *)this->sc->yuv;
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
bool VideoInput::InitAVCodecContext() {
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
  this->sc->av_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;  //(AVPixelFormat)this->dst_video_format;
  return true;
}

VideoInput::~VideoInput() {
  std::cout << "~VideoInput() : Release VideoInput instance start" << std::endl;
  if (this->sc->sws_context) {
    sws_freeContext(this->sc->sws_context);
    this->sc->sws_context = nullptr;
  }
  if (this->sc->yuv) {
    av_frame_free(&this->sc->yuv);
  }
  if (this->sc->av_codec_context) {
    avcodec_free_context(&this->sc->av_codec_context);
  }
  av_packet_unref(&this->sc->av_pack);

  std::cout << "~VideoInput() : Release VideoInput instance end" << std::endl;
}

/***************************************************
 *                      Audio                      *
 ***************************************************/
AudioInput *AudioInput::Get(AudioStreamingContext *audio_sc) {
  static AudioInput instance(audio_sc);
  return &instance;  // return pointer
}

AudioInput::AudioInput(AudioStreamingContext *audio_sc) {
  this->sc = *audio_sc;
}

// AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_S16
bool AudioInput::InitContext(enum AVSampleFormat output_format, enum AVSampleFormat input_format) {
  this.sc->swr_context = nullptr;
  this.sc->swr_context = swr_alloc_set_opts(this->sc->swr_context,
                                            av_get_default_channel_layout(this->sc->channels), output_format, this->sc->sample_rate,  // Output format
                                            av_get_default_channel_layout(this->sc->channels), input_format, this->sc->sample_rate,
                                            0, nullptr);
  if (!this->sc->swr_context) {
    std::cout << "Error: AudioInput::InitContext()" << std::endl;
    std::cout << "  -> swr_alloc_set_opts()" << std::endl;
    return false;
  }
  int ret = swr_init(this->sc->swr_context);
  if (ret != 0) {
    std::cout << "Error: AudioInput::InitContext()" << std::endl;
    std::cout << "  -> swr_init()" << std::endl;
    return ErrorMesssage(ret);
  }
  this->output_sample_format = output_format;
  this->input_sample_format = input_format;
  return true;
}
bool AudioInput::InitAndOpenAVCodecContext(enum AVCodecID av_codec_id) {
  //AV_CODEC_ID_AAC
  // init AVCodec
  this->sc->av_codec = avcodec_find_encoder(av_codec_id);
  if (!this->sc->av_codec) {
    std::cout << "Error: AudioInput::InitAndOpenAVCodecContext()" << std::endl;
    std::cout << "  -> avcodec_find_encoder()" << std::endl;
    return false;
  }
  // Init AVCodecContext
  this->sc->av_codec_context = avcodec_alloc_context3(this->sc->av_codec);
  if (!this->sc->av_codec_context) {
    std::cout << "Error: AudioInput::InitAndOpenAVCodecContext()" << std::endl;
    std::cout << "  -> avcodec_alloc_context3()" << std::endl;
    return false;
  }
  // AVCodecContext setting
  this->sc->av_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  this->sc->av_codec_context->thread_count = GetCpuNum();
  this->sc->av_codec_context->time_base = {1, 1000000};

  this->sc->av_codec_context->bit_rate = 40000;
  this->sc->av_codec_context->sample_rate = this->sc->sample_rate;
  this->sc->av_codec_context->sample_fmt = (AVSampleFormat)this->output_sample_format;
  this->sc->av_codec_context->channels = this->sc->channels;
  this->sc->av_codec_context->channel_layout = av_get_default_channel_layout(this->sc->channels);

  // copy AVCodec parameters to AVCodecContext
  std::future<int> fuRes_ = std::async(std::launch::async, avcodec_open2,
                                       this->sc->av_codec_context, nullptr, nullptr);
  int ret = fuRes_.get();
  if (ret != 0) {
    std::cout << "AudioInput::InitAndOpenAVCodecContext()" << std::endl;
    std::cout << "  -> avcodec_open2()" << std::endl;
    avcodec_free_context(&this->sc->av_codec_context);
    return ErrorMesssage(ret);
  }
  return true;
}

bool AudioInput::InitAndGetAVFrameFromData() {
  // init && allocate AVFrame
  this->sc->pcm = av_frame_alloc();
  this->sc->pcm->format = this->output_sample_format;
  this->sc->pcm->channels = this->sc->channels;
  this->sc->pcm->channel_layout = av_get_default_channel_layout(this->sc->channels);
  this->sc->pcm->nb_samples = this->sc->nb_samples;
  int ret = av_frame_get_buffer(this->sc->pcm, 0);
  if (ret != 0) {
    std::cout << "Error: AudioInput::InitAndGetAVFrameFromData()" << std::endl;
    std::cout << "  -> av_frame_get_buffer()" << std::endl;
    return ErrorMesssage(ret);
  }
  return true;
}

Data AudioInput::Resample(Data src_data) {
  Data tmp;
  if (src_data.size <= 0 || !src_data.data) {
    std::cout << "Error: AudioInput::Resample()" << std::endl;
    std::cout << "  -> (Data) src_data.size <= 0 || src_data.data=nullptr" << std::endl;
    return tmp;
  }
  const uint8_t *input_data[AV_NUM_DATA_POINTERS] = {0};
  input_data[0] = (uint8_t *)src_data.data;
  int len = swr_convert(this->sc->swr_context, this->sc->pcm->data, this->sc->pcm->nb_samples,
                        input_data, this->sc->pcm->nb_samples);
  if (len <= 0) {
    std::cout << "Error: AudioInput::Resample()" << std::endl;
    std::cout << "  -> swr_convert()" << std::endl;
    return tmp;
  }
  this->sc->pcm->pts = src_data.pts;  // copy time_base from src_data to AVFrame
  tmp.data = (char *)(this->sc->pcm);
  tmp.size = this->sc->pcm->nb_samples * this->sc->pcm->channels * 2;
  tmp.pts = src_data.pts;
  return tmp;
}

// According to AVCodecContext to encode the AVPacket
Data AudioInput::EncodeAudio(Data src_data) {
  Data tmp;
  if (src_data.size <= 0 || !src_data.data) {
    std::cout << "Error: AudioInput::EncodeAudio()" << std::endl;
    std::cout << "  -> (Data) src_data.size <= 0 || src_data.data=nullptr" << std::endl;
    return tmp;
  }
  AVFrame *p_av_frame = (AVFrame *)src_data.data;
  if (this->sc->lastPts == p_av_frame->pts) {
    p_av_frame->pts += 1000;
  }
  this->sc->lastPts = p_av_frame->pts;

  // passing data from AVFrame to AVCodecContext
  int ret = avcodec_send_frame(this->sc->av_codec_context, p_av_frame);
  if (ret != 0) {
    std::cout << "Error: AudioInput::EncodeAudio()" << std::endl;
    std::cout << "  -> avcodec_send_frame()" << std::endl;
    return tmp;
  }
  // receive data from buffer(AVCodecContext). if return 0 : can receice data
  av_packet_unref(&this->sc->av_pack);
  ret = avcodec_receive_packet(this->sc->av_codec_context, &this->sc->av_pack);
  if (ret != 0) {
    std::cout << "Error: Error: AudioInput::EncodeAudio()" << std::endl;
    std::cout << "  -> avcodec_receive_packet()" << std::endl;
    return tmp;
  }
  tmp.data = (char *)&this->sc->av_pack;  // copy data from encoded av_pack (AVFrame)
  tmp.size = this->sc->av_pack.size;      // copy size from encoded av_pack (AVFrame)
  tmp.pts = src_data.pts;                 // copy pts from original src_data (Data)
  return tmp;
}

AudioInput::~AudioInput() {
  std::cout << "~AudioInput() : Release AudioInput instance start" << std::endl;
  if (this->sc->swr_context) {
    swr_free(&this->sc->swr_context);
  }
  if (this->sc->pcm) {
    av_frame_free(&this->sc->pcm);
  }
  av_packet_unref(&this->sc->av_pack);

  std::cout << "~AudioInput() : Release AudioInput instance start" << std::endl;
}
