#include "InputStream.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <future>
#include <iostream>

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

static int GetCpuNum() {
#if defined WIN32 || defined _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return (int)sysinfo.dwNumberOfProcessors;
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

/************************************
 *          VideoInputFactory       *
*************************************/
class VideoInput : public VideoInputFactory {
 public:
  // VideoInput() {  // initialize all parameter
  //   sws_context = nullptr;
  //   yuv = nullptr;
  //   video_packet = {0};
  // }

  // ~VideoInput() {  // Release all memory
  //   if (sws_context) {
  //     sws_freeContext(sws_context);
  //     sws_context = nullptr;
  //   }
  //   if (yuv) {
  //     av_frame_free(&yuv);
  //   }
  //   av_packet_unref(&video_packet);
  //   std::cout << "VideoInputFactory Release success." << std::endl;
  // }

  void Close() {
    if (sws_context) {
      sws_freeContext(sws_context);
      sws_context = nullptr;
    }
    if (yuv) {
      av_frame_free(&yuv);
    }
    av_packet_unref(&video_packet);
    std::cout << "VideoInputFactory Release success." << std::endl;
  }

  // AV_PIX_FMT_BGR24, AV_PIX_FMT_YUV420P
  bool InitContext(enum AVPixelFormat src_format, enum AVPixelFormat dst_format) {
    this->src_pixel_format = static_cast<int>(src_format);
    this->dst_pixel_format = static_cast<int>(dst_format);
    sws_context = sws_getCachedContext(this->sws_context,
                                       this->src_img_width, this->src_img_height, AV_PIX_FMT_BGR24,
                                       this->dst_img_width, this->dst_img_height, AV_PIX_FMT_YUV420P,
                                       SWS_BICUBIC,
                                       0, 0, 0);
    if (!this->sws_context) {
      std::cout << "ERROR: bool VideoInputFactory::InitContext()" << std::endl;
      std::cout << "  -> sws_getCachedContext() failed." << std::endl;
      return false;
    }
    return true;
  }

  // AV_CODEC_ID_H264
  bool InitAndOpenAVCodecContext(enum AVCodecID video_codec) {
    // AV_CODEC_ID_H264
    this->codec_id = static_cast<int>(video_codec);
    // AVCodec // (AVCodecID)this->codec_id
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
      std::cout << "Error: bool VideoInputFactory::InitAndOpenAVCodecContext()" << std::endl;
      std::cout << " -> avcodec_find_encoder() failed." << std::endl;
      return false;
    }
    // AVCodecContext
    this->av_codec_context = avcodec_alloc_context3(codec);

    this->av_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    this->av_codec_context->thread_count = GetCpuNum();
    this->av_codec_context->time_base = {1, 1000000};

    this->av_codec_context->bit_rate = 50 * 1024 * 8;
    this->av_codec_context->width = this->dst_img_width;
    this->av_codec_context->height = this->dst_img_height;
    this->av_codec_context->framerate = {this->fps, 1};
    this->av_codec_context->gop_size = 50;
    this->av_codec_context->max_b_frames = 0;
    this->av_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;  //(AVPixelFormat)this->dst_pixel_format;
    // open AVCodecContext
    std::future<int> fuRes = std::async(std::launch::async, avcodec_open2, this->av_codec_context, nullptr, nullptr);
    int ret = fuRes.get();
    if (ret != 0) {
      std::cout << "Error: bool VideoInputFactory::InitAndOpenAVCodecContext()" << std::endl;
      std::cout << "  -> avcodec_open2() failed." << std::endl;
      return ErrorMesssage(ret);
    }
    return true;
  }

  bool InitAVFrame() {
    this->yuv = av_frame_alloc();
    this->yuv->format = AV_PIX_FMT_YUV420P;
    this->yuv->width = this->src_img_width;
    this->yuv->height = this->src_img_height;
    this->yuv->pts = 0;
    int ret = av_frame_get_buffer(this->yuv, 32);
    if (ret != 0) {
      std::cout << "bool VideoInputFactory::InitAVFrame()" << std::endl;
      std::cout << "  -> av_frame_get_buffer() failed." << std::endl;
      return ErrorMesssage(ret);
    }
    return true;
  }

  Data ConvertPixelFormat(Data rgb) {
    // Data tmp;
    // if (src_data.size <= 0 || !src_data.data) {
    //   std::cout << "Error: VideoInputFactory::ConvertPixelFormat" << std::endl;
    //   std::cout << "  -> (Data) src_data.size<=0 || src_data.data=nullptr" << std::endl;
    //   return tmp;
    // }
    // tmp.pts = src_data.pts;
    // uint8_t *input_data[AV_NUM_DATA_POINTERS] = {0};
    // input_data[0] = (uint8_t *)src_data.data;  // copy src_data to uint8_t (char *) -> (uint8_t *)
    // int input_size[AV_NUM_DATA_POINTERS] = {0};
    // input_size[0] = this->src_img_width * this->pixel_size;
    // int h = sws_scale(this->sws_context,
    //                   input_data, input_size, 0, this->src_img_height,
    //                   this->yuv->data, this->yuv->linesize);
    // if (h <= 0)
    //   return tmp;
    // this->yuv->pts = src_data.pts;
    // tmp.data = (char *)this->yuv;
    // int *p = this->yuv->linesize;
    // while ((*p)) {
    //   tmp.size += (*p) * this->dst_img_height;
    //   p++;
    // }
    // return tmp;
    Data tmp;
    tmp.pts = rgb.pts;
    //Input data
    uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
    //indata[0] bgrbgrbgr
    //plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
    indata[0] = (uint8_t *)rgb.data;
    int insize[AV_NUM_DATA_POINTERS] = {0};

    insize[0] = this->src_img_width * this->pixel_size;

    int h = sws_scale(this->sws_context, indata, insize, 0, this->src_img_height,
                      yuv->data, yuv->linesize);
    if (h <= 0) {
      return tmp;
    }
    yuv->pts = rgb.pts;
    tmp.data = (char *)yuv;
    int *p = yuv->linesize;
    while ((*p)) {
      tmp.size += (*p) * this->dst_img_height;
      p++;
    }
    return tmp;
  }

  Data EncodeVideo(Data frame) {
    // Data tmp;
    // if (src_data.size <= 0 || !src_data.data) {
    //   std::cout << "Error: VideoInputFactory::EncodeVideo" << std::endl;
    //   std::cout << "  -> (Data) src_data.size<=0 || src_data.data=nullptr" << std::endl;
    //   return tmp;
    // }
    // AVFrame *p_av_frame = (AVFrame *)src_data.data;
    // // passing data from AVFrame to AVCodecContext
    // int ret = avcodec_send_frame(this->av_codec_context, p_av_frame);
    // if (ret != 0) {
    //   std::cout << "Error: VideoInput::EncodeVideo" << std::endl;
    //   return tmp;
    // }
    // av_packet_unref(&video_packet);
    // ret = avcodec_receive_packet(this->av_codec_context, &video_packet);
    // if (ret != 0 || video_packet.size <= 0) {
    //   return tmp;
    // }
    // tmp.data = (char *)&video_packet;  // copy
    // tmp.size = video_packet.size;      // copy
    // tmp.pts = src_data.pts;            // get time from src_data
    // return tmp;
    std::cout << "XXXXXXXXXXXX frame.size : " << frame.size << std::endl;
    av_packet_unref(&video_packet);

    Data tmp;
    if (frame.size <= 0 || !frame.data)
      return tmp;
    AVFrame *p = (AVFrame *)frame.data;
    // h264 encoding
    int ret = avcodec_send_frame(this->av_codec_context, p);
    if (ret != 0)
      return tmp;

    ret = avcodec_receive_packet(this->av_codec_context, &video_packet);
    if (ret != 0 || video_packet.size <= 0)
      return tmp;
    tmp.data = (char *)&video_packet;
    tmp.size = video_packet.size;
    tmp.pts = frame.pts;
    std::cout << "XXXXXXXXXXXX tmp.size : " << tmp.size << std::endl;
    return tmp;
  }

 private:
  SwsContext *sws_context = nullptr;  // be use to convert pixel format &  frame size
  AVFrame *yuv = nullptr;
  AVPacket video_packet = {0};
};

VideoInputFactory *VideoInputFactory::Get(unsigned char index) {
  static VideoInput vf_instance[255];
  return &vf_instance[index];
}

/************************************
 *          AudioInputFactory       *
*************************************/
class AudioInput : public AudioInputFactory {
 public:
  // AudioInput() {  // initialize
  //   swr_context = nullptr;
  //   pcm = nullptr;
  //   audio_packet = {0};
  // }
  // ~AudioInput() {  // Release
  //   if (swr_context) {
  //     swr_free(&this->swr_context);
  //   }
  //   if (pcm) {
  //     av_frame_free(&this->pcm);
  //   }
  //   av_packet_unref(&this->audio_packet);
  //   std::cout << "AudioInputFactory Release success." << std::endl;
  // }
  void Close() {
    if (swr_context) {
      swr_free(&this->swr_context);
    }
    if (pcm) {
      av_frame_free(&this->pcm);
    }
    av_packet_unref(&this->audio_packet);
    std::cout << "AudioInputFactory Release success." << std::endl;
  }

  // AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_S16
  bool InitContext(enum AVSampleFormat output_format, enum AVSampleFormat input_format) {
    this->dst_sample_format = static_cast<int>(output_format);
    this->src_sample_format = static_cast<int>(input_format);
    this->swr_context = nullptr;
    this->swr_context = swr_alloc_set_opts(this->swr_context,
                                           av_get_default_channel_layout(this->channels), (AVSampleFormat)this->dst_sample_format, this->sample_rate,
                                           av_get_default_channel_layout(this->channels), (AVSampleFormat)this->src_sample_format, this->sample_rate,
                                           0, 0);
    if (!this->swr_context) {
      std::cout << "Error: bool AudioInputFactory::InitContext()" << std::endl;
      std::cout << "  -> swr_alloc_set_opts() failed." << std::endl;
      return false;
    }
    int ret = swr_init(this->swr_context);
    if (ret != 0) {
      std::cout << "Error: bool AudioInputFactory::InitContext()" << std::endl;
      std::cout << "  -> swr_init() failed." << std::endl;
      return ErrorMesssage(ret);
    }
    return true;
  }
  bool InitAndOpenAVCodecContext(enum AVCodecID av_codec_id) {
    this->codec_id = static_cast<int>(av_codec_id);
    // AVCodec
    AVCodec *codec = avcodec_find_encoder((AVCodecID)this->codec_id);
    if (!codec) {
      std::cout << "Error: bool AudioInputFactory::InitAndOpenAVCodecContext()" << std::endl;
      std::cout << "  -> avcodec_find_encoder() failed." << std::endl;
      return false;
    }
    // AVCodecContext
    this->av_codec_context = avcodec_alloc_context3(codec);
    if (!this->av_codec_context) {
      std::cout << "Error: bool AudioInputFactory::InitAndOpenAVCodecContext()" << std::endl;
      std::cout << "  -> avcodec_alloc_context3() failed." << std::endl;
      return false;
    }
    this->av_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    this->av_codec_context->thread_count = GetCpuNum();
    this->av_codec_context->time_base = {1, 1000000};

    this->av_codec_context->bit_rate = 40000;
    this->av_codec_context->sample_rate = this->sample_rate;
    this->av_codec_context->sample_fmt = (AVSampleFormat)this->dst_sample_format;
    this->av_codec_context->channels = this->channels;
    this->av_codec_context->channel_layout = av_get_default_channel_layout(this->channels);
    // open AVCodecContext
    std::future<int> fuRes = std::async(std::launch::async, avcodec_open2, this->av_codec_context, nullptr, nullptr);
    int &&ret = fuRes.get();
    if (ret != 0) {
      std::cout << "Error: bool AudioInputFactory::InitAndOpenAVCodecContext()" << std::endl;
      std::cout << "  -> avcodec_open2() failed." << std::endl;
      return ErrorMesssage(ret);
    }
    return true;
  }
  bool InitAVFrame() {
    this->pcm = av_frame_alloc();
    this->pcm->format = (AVSampleFormat)this->dst_sample_format;
    this->pcm->channels = this->channels;
    this->pcm->channel_layout = av_get_default_channel_layout(this->channels);
    this->pcm->nb_samples = this->nb_sample;  // Sample number per frame in one channel
    int ret = av_frame_get_buffer(pcm, 0);    // allocation for pcm
    if (ret != 0) {
      std::cout << "Error: bool AudioInputFactory::InitAVFrame()" << std::endl;
      std::cout << "  -> av_frame_get_buffer() failed." << std::endl;
      return ErrorMesssage(ret);
    }
    return true;
  }
  Data Resample(Data src_data) {
    Data tmp;
    const uint8_t *input_data[AV_NUM_DATA_POINTERS] = {0};
    input_data[0] = (uint8_t *)src_data.data;
    int len = swr_convert(this->swr_context, this->pcm->data, this->pcm->nb_samples,
                          input_data, this->pcm->nb_samples);
    if (len <= 0) {
      return tmp;
    }
    this->pcm->pts = src_data.pts;
    tmp.data = (char *)this->pcm;
    tmp.size = this->pcm->nb_samples * this->channels * 2;
    tmp.size = src_data.pts;
    return tmp;
  }
  Data EncodeAudio(Data src_data) {
    Data tmp;
    if (src_data.size <= 0 || !src_data.data) {
      std::cout << "Error: AudioInputFactory::EncodeAudio" << std::endl;
      std::cout << "  -> (Data) src_data.size<=0 || src_data.data=nullptr" << std::endl;
      return tmp;
    }
    AVFrame *p = (AVFrame *)src_data.data;
    if (lastPts == p->pts) {
      p->pts += 1000;
    }
    lastPts = p->pts;
    int ret = avcodec_send_frame(this->av_codec_context, p);
    if (ret != 0)
      return tmp;
    av_packet_unref(&this->audio_packet);
    ret = avcodec_receive_packet(this->av_codec_context, &this->audio_packet);
    if (ret != 0)
      return tmp;
    tmp.data = (char *)&this->audio_packet;
    tmp.size = this->audio_packet.size;
    tmp.pts = src_data.pts;
    return tmp;
  }

 private:
  long long lastPts = -1;
  SwrContext *swr_context = nullptr;
  AVFrame *pcm = nullptr;
  AVPacket audio_packet = {0};
};
AudioInputFactory *AudioInputFactory::Get(unsigned char index) {
  static AudioInput af_instance[255];
  return &af_instance[index];
}