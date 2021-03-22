#include <future>
#include <iostream>
#include <opencv2/highgui.hpp>

#include "pch.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

using namespace std;

// nginx-rtmp url
static const std::string outUrl = "rtmp://127.0.0.1:443/live/home";

int ErrorMesssage(const int &error_num) {
  char buf[1024] = {0};
  // put error code to buf
  av_strerror(error_num, buf, sizeof(buf));
  cout << buf << endl;
  return -1;
}

/*
if input context is NULL will allocate memory(calls sws_getContext() to get a new context).
*/
int GetContextStream(SwsContext *context,
                     int src_img_width, int src_img_height, enum AVPixelFormat src_pixel_format,
                     int dst_img_width, int dst_img_height, enum AVPixelFormat dst_pixel_format,
                     int flags, SwsFilter *src_filter, SwsFilter *dst_filter, const double *param) {
  context = sws_getCachedContext(context,
                                 src_img_width, src_img_height, AVPixelFormat::AV_PIX_FMT_BGR24,
                                 dst_img_width, dst_img_height, AVPixelFormat::AV_PIX_FMT_YUV420P,
                                 SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
  if (!context) {
    std::cout << "ERROR: GetContextStream()" << std::endl;
    std::cout << "  ->sws_getCachedContext() failed." << std::endl;
    return -1;
  }
}

// reate && Allocate memory to AVCodecContext. release by av_frame_free()
int InitializeOutputAVFrame(AVFrame *av_frame, const int &img_width, const int &img_height, enum AVPixelFormat pixel_format) {
  /*
  Three ways for allocating memory
  https://blog.csdn.net/xionglifei2014/article/details/90693048
  */
  // AVFrame : This structure describes decoded (raw) audio or video data.
  // allocate AVFrame and set attributes to default value
  av_frame = av_frame_alloc();
  av_frame->format = pixel_format;
  av_frame->width = img_width;
  av_frame->height = img_height;
  av_frame->pts = 0;
  // allocate the new buffer for audio or video data.
  const int &&ret = av_frame_get_buffer(av_frame, 32);
  if (ret != 0)
    return ErrorMesssage(ret);
  return 0;
}

// Create && Allocate memory to AVCodecContext. release by avcodec_free_context()
int InitializeAVCodecContext(AVCodecContext *avCodecContext, const int &img_width, const int &img_height,
                             const int &fps, enum AVPixelFormat pixel_format) {
  // Find a registered encoder with a matching codec ID.
  AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec)
    return -1;
  // Allocate an AVCodecContext and set its fields to default values.
  avCodecContext = avcodec_alloc_context3(codec);
  if (!avCodecContext) {
    std::cout << "ERROR: InitializeAVCodecContext()" << std::endl;
    std::cout << "  -> avcodec_alloc_context3() failed." << std::endl;
    return -1;
  }
  // set parameters for AVCodecContext
  avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  //全局参数
  avCodecContext->codec_id = codec->id;
  avCodecContext->thread_count = 8;

  avCodecContext->bit_rate = 50 * 1024 * 8;  // 壓縮後每秒frame的bit位元大小 50kB
  avCodecContext->width = img_width;
  avCodecContext->height = img_height;
  avCodecContext->time_base = {1, fps};
  avCodecContext->framerate = {fps, 1};

  avCodecContext->gop_size = 50;
  avCodecContext->max_b_frames = 0;
  avCodecContext->pix_fmt = pixel_format;
  // Open codec context (Two ways can do it.)
  // std::future<int> fuRes = std::async(std::launch::async, avcodec_open2, avCodecContext, nullptr, nullptr);
  // const int &&ret = fuRes.get();
  const int &&ret = avcodec_open2(avCodecContext, nullptr, nullptr);
  if (ret != 0) {
    return ErrorMesssage(ret);
  }
  std::cout << "InitializeAVCodecContext() success" << std::endl;
  return 0;
}

// Allocate an AVFormatContext for an output format.
int InitializeAVFormatContextAndCreateNewStream(AVFormatContext *avFormatContext, AVCodecContext *avCodecContext, AVStream *av_stream) {
  int &&ret = avformat_alloc_output_context2(&avFormatContext, nullptr, "flv", outUrl.c_str());
  if (ret != 0) {
    return ErrorMesssage(ret);
  }
  // Add a new stream to a media file.
  av_stream = avformat_new_stream(avFormatContext, nullptr);
  if (!av_stream) {
    std::cout << "ERROR: InitializeAVFormatContextAndCreateNewStream()" << std::endl;
    std::cout << "  avformat_new_stream() failed." << std::endl;
    return -1;
  }
  av_stream->codecpar->codec_tag = 0;
  // Fill the parameters struct based on the values from the supplied codeccontext.
  avcodec_parameters_from_context(av_stream->codecpar, avCodecContext);
  av_dump_format(avFormatContext, 0, outUrl.c_str(), 1);
}

//
int OutputVideoStreamRTMP(cv::VideoCapture &cam, AVFormatContext *avFormatContext,
                          AVFrame *yuv, SwsContext *swsContext, AVCodecContext *avCodecContext,
                          AVStream *avStream, cv::Mat &frame) {
  int &&ret = avio_open(&avFormatContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE);
  if (ret != 0)
    return ErrorMesssage(ret);
  ret = avformat_write_header(avFormatContext, nullptr);
  if (ret != 0)
    return ErrorMesssage(ret);
  AVPacket pack;
  memset(&pack, 0, sizeof(pack));
  int vpts = 0;
  while (true) {
    // Grabs the next frame from video file or capturing device.
    if (!cam.grab()) {
      continue;
    }
    // Decodes and returns the grabbed video frame.
    if (!cam.retrieve(frame)) {
      continue;
    }
    cv::imshow("Camera Image", frame);
    char key = cv::waitKey(1);
    if (key == 'q') break;

    // rgb to yuv
    uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
    indata[0] = frame.data;
    int insize[AV_NUM_DATA_POINTERS] = {0};
    // column size of data
    insize[0] = frame.cols * frame.elemSize();
    int h = sws_scale(swsContext, indata, insize, 0, frame.rows,  // data source
                      yuv->data, yuv->linesize);
    if (h <= 0)
      continue;
    ///h264 encode
    yuv->pts = vpts;
    vpts++;
    ret = avcodec_send_frame(avCodecContext, yuv);
    if (ret != 0)
      continue;
    ret = avcodec_receive_packet(avCodecContext, &pack);
    if (ret != 0)
      continue;

    // Upstream
    pack.pts = av_rescale_q(pack.pts, avCodecContext->time_base, avStream->time_base);
    pack.dts = av_rescale_q(pack.dts, avCodecContext->time_base, avStream->time_base);
    pack.duration = av_rescale_q(pack.duration, avStream->time_base, avStream->time_base);
    ret = av_interleaved_write_frame(avFormatContext, &pack);
  }
}

void Release(cv::VideoCapture &cam, SwsContext *swsContext, AVFrame *yuv,
             AVCodecContext *avCodecContext, AVStream *avStream, AVFormatContext *avFormatContext) {
  if (cam.isOpened())
    cam.release();
  if (swsContext) {
    sws_freeContext(swsContext);
    swsContext = nullptr;
  }
  if (yuv)
    av_frame_free(&yuv);
  if (avCodecContext)
    avcodec_free_context(&avCodecContext);
  if (avFormatContext)
    avformat_free_context(avFormatContext);
}

int main() {
  // Register all network protocols
  avformat_network_init();
  //==============intialize all object================
  // Context for pixel and format transformation
  SwsContext *swsContext = nullptr;
  // Struct of output data (decode data)
  AVFrame *yuv = nullptr;
  // Context for codec
  AVCodecContext *avCodecContext = nullptr;
  // Video stream
  AVStream *avStream;
  // Format IO context
  AVFormatContext *avFormatContext = nullptr;

  //==============Open web camera================
  cv::VideoCapture cam;
  cv::Mat frame;
  cv::namedWindow("Camera Image");
  if (!cam.open(0, cv::CAP_DSHOW)) {
    std::cout << "Webcam open failed!" << std::endl;
    return -1;
  }
  //==============Get input frame info from camera, and get context via GetContextStream()================
  // cam.set(cv::CAP_PROP_FPS, 30);
  // int fps = cam.get(cv::CAP_PROP_FPS);
  const int &&fps = 30;
  int img_width = cam.get(cv::CAP_PROP_FRAME_WIDTH);
  int img_height = cam.get(cv::CAP_PROP_FRAME_HEIGHT);
  if (GetContextStream(swsContext,
                       img_width, img_height, AVPixelFormat::AV_PIX_FMT_BGR24,
                       img_width, img_height, AVPixelFormat::AV_PIX_FMT_YUV420P,
                       SWS_FAST_BILINEAR, nullptr, nullptr, nullptr) != 0) {
    return -1;
  }

  //==============Allocate memory to AVFrame (yuv)================
  if (InitializeOutputAVFrame(yuv, img_width, img_height, AVPixelFormat::AV_PIX_FMT_YUV420P) != 0) {
    return -1;
  }
  //==============Allocate memory to avCodecContext (yuv)================
  if (InitializeAVCodecContext(avCodecContext, img_width, img_height, fps, AVPixelFormat::AV_PIX_FMT_YUV420P) != 0) {
    return -1;
  }
  //==============Allocate memory to AVFormatContext (avFormatContext)================
  if (InitializeAVFormatContextAndCreateNewStream(avFormatContext, avCodecContext, avStream) != 0)
    return -1;
  //==============OutputVideoStreamRTMP================
  if (OutputVideoStreamRTMP(cam, avFormatContext, yuv, swsContext, avCodecContext,
                            avStream, frame) != 0)
    return -1;
  //==============Release================
  Release(cam, swsContext, yuv, avCodecContext, avStream, avFormatContext);

  return 0;
}