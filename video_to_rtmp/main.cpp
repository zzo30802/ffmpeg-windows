#include <chrono>
#include <iostream>
#include <thread>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

using namespace std;

int ErrorMesssage(const int& error_num) {
  char buf[1024] = {0};
  // put error code to buf
  av_strerror(error_num, buf, sizeof(buf));
  cout << buf << endl;
  return -1;
}

static double r2d(AVRational r) {
  return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int GetFileStreamInfo(AVFormatContext*& input_format_context, const char* file_path) {
  /*
  AVFormatContext *& in: allocated by avformat_alloc_context
  const char* : Name of the stream to open.
  */
  // check file path
  if (!file_path) {
    cout << "ERROR: GetFileStreamInfo() input file_path null" << endl;
    return -1;
  }
  // Allocate memory && Open an input stream and read the header to AVFormatContext.
  int re = avformat_open_input(&input_format_context, file_path, 0, 0);
  if (re != 0)
    return ErrorMesssage(re);
  cout << "open file " << file_path << "success." << endl;

  // Read packets of a media file to get stream information.
  re = avformat_find_stream_info(input_format_context, 0);
  if (re != 0)
    return ErrorMesssage(re);

  // Print detailed information about the input or output format, such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base.
  std::cout << "=====Print input stream infomations=====" << std::endl;
  av_dump_format(input_format_context, 0, file_path, 0);
  std::cout << "========================================" << std::endl;
  return 0;
}

/*
https://welkinchen.pixnet.net/blog/post/5419657-ffmpeg-%E4%BD%BF%E7%94%A8%E7%AD%86%E8%A8%98
// AVFormat 
- 每種AVFormat代表一種格式，像是.avi、.wmv、.dat 等等各種格式，使用不同的方式 mux/demux 多個 stream。
- 每一個 AVFormat 都實作特定的格式，使用時必需選擇和目的格式相符的 AVFormat 實作，才能正確的讀取 stream 的內容。
// AVFormatContext
AVFormatContext 是 AVFormat 的一個 instance，用以存放 AVFormat 的執行狀態。
使用 FFmpeg 處理檔案時，必需為 AVInputFormat/AVOutputFormat 建立一個 AVFormatContext。
同一個 format 可以建立多個 AVFormatContext，各立獨立，不相互干擾，以同時處理多個檔案。
// AVStream
AVStream 用以對應到 AVFormatContext 裡的一個 stream。
因此同一個 AVFormatContext 管理了多個 AVStream，以對應到存在檔案或透過 network session 傳送的數個 stream。
*/
int CreateAVFormatContextInstance(AVFormatContext*& output_format_context, const char* format_type, const char* out_url) {
  // Allocate an AVFormatContext for an output format
  int res = avformat_alloc_output_context2(&output_format_context, 0, format_type, out_url);
  if (!output_format_context) {
    return ErrorMesssage(res);
  }
  cout << "AVFormatContext instance create success!" << endl;
  return 0;
}

int CreateOutputStream(AVFormatContext*& input_format_context, AVFormatContext*& output_format_context) {
  // According to input AVFormatContext that contains how many streams, copy them to a new AVFormatContext.
  for (int i = 0; i < input_format_context->nb_streams; i++) {
    // Find a registered encoder with a matching codec ID.
    AVCodec* codec_in = avcodec_find_encoder(input_format_context->streams[i]->codecpar->codec_id);
    // Add a new stream to a media file. (will return error if the newly created stream or NULL)
    AVStream* out_stream = avformat_new_stream(output_format_context, codec_in);
    if (!out_stream)
      return ErrorMesssage(0);
    cout << "avformat_new_stream success!" << endl;
    // Copy the contents of src to dst.
    int res = avcodec_parameters_copy(out_stream->codecpar, input_format_context->streams[i]->codecpar);
    if (res != 0)
      return ErrorMesssage(res);
    codec_in->codec_tags = 0;
  }
  return 0;
}

// According to input url to create and initialize a new struct that cotains "Bytestream IO Context".
int CreateAndInitAVIOContext(AVFormatContext*& output_format_context, const char* url) {
  // Create and initialize a AVIOContext for accessing the resource indicated by url.
  int res = avio_open(&output_format_context->pb, url, AVIO_FLAG_WRITE);
  if (res < 0) {
    std::cout << "Could not open output file:" << url << std::endl;
    return ErrorMesssage(res);
  }
  // Allocate the stream private data and write the stream header to an output media file.
  res = avformat_write_header(output_format_context, 0);
  if (res < 0) {
    std::cout << "Error occurred when opening output file" << std::endl;
    return ErrorMesssage(res);
  }
  cout << "avformat_write_header : " << res << endl;
  return 0;
}

// Muxers take encoded data in the form of AVPackets and write it into files or other output bytestreams in the specified container format.
int Remuxing(AVFormatContext*& input_format_context, AVFormatContext*& output_format_context) {
  // This structure stores compressed data.
  AVPacket pkt;
  std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();  //av_gettime();
  while (true) {
    int res = av_read_frame(input_format_context, &pkt);
    if (res != 0)
      break;
    cout << pkt.pts << " " << flush;
    //
    AVRational itime = input_format_context->streams[pkt.stream_index]->time_base;
    AVRational otime = output_format_context->streams[pkt.stream_index]->time_base;
    // Rescale a 64-bit integer by 2 rational numbers with specified rounding.
    pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
    pkt.dts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
    pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
    pkt.pos = -1;
    // Speed Control
    if (input_format_context->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      AVRational tb = input_format_context->streams[pkt.stream_index]->time_base;
      // Past time
      long long now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();
      long long dts = 0;
      dts = pkt.dts * (1000 * 1000 * r2d(tb));
      if (dts > now)
        std::this_thread::sleep_for(std::chrono::microseconds(dts - now));
    }

    res = av_interleaved_write_frame(output_format_context, &pkt);
    if (res < 0) {
      return ErrorMesssage(res);
    }
  }
  return 0;
}

int main() {
  cout << "main start" << endl;

  const char* inUrl = "../video/SampleVideo.flv";          // input file
  const char* outUrl = "rtmp://127.0.0.1:1935/live/home";  // ouput url
  // const char* outUrl = "rtmp://127.0.0.1:443/live/home";
  const char* output_format = "flv";  // output format

#pragma region Get Information from Input file
  AVFormatContext* input_context = NULL;
  if (GetFileStreamInfo(input_context, inUrl) != 0)
    return -1;
#pragma endregion

#pragma region OutputStream
  AVFormatContext* output_context = NULL;
  if (CreateAVFormatContextInstance(output_context, output_format, outUrl) != 0)
    return -1;
  if (CreateOutputStream(input_context, output_context) != 0)
    return -1;
  // Print detailed information about the input or output format, such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base.
  std::cout << "=======Print RTMP stream detailed=======" << std::endl;
  av_dump_format(output_context, 0, outUrl, 1);
  std::cout << "========================================" << std::endl;
#pragma endregion

#pragma region RTMP
  if (CreateAndInitAVIOContext(output_context, outUrl) != 0)
    return -1;
  if (Remuxing(input_context, output_context) != 0)
    return -1;
#pragma endregion

  std::cout << std::endl
            << "main end" << std::endl;
  return 0;
}