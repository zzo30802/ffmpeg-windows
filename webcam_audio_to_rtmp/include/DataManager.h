#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_
#include <QtCore/QThread>
#include <iostream>
#include <mutex>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

static bool ErrorMesssage(const int &error_num) {
  char buf[1024]{0};
  // put error code to buf
  av_strerror(error_num, buf, sizeof(buf));
  std::cout << buf << std::endl;
  return false;
}

typedef struct VideoStreamingContext {
  int img_width = 1280;
  int img_height = 720;
  int pixel_size = 3;
  int fps = 25;
  SwsContext *sws_context = nullptr;
  AVFormatContext *av_format_context = nullptr;
  AVCodec *av_codec = nullptr;
  AVStream *av_stream = nullptr;
  AVCodecContext *av_codec_context = nullptr;
  AVFrame *yuv = nullptr;
  AVPacket av_pack = {0};
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
  AVPacket av_pack = {0};
  int index;
  char *outUrl;
} AudioStreamingContext;

struct Data {
 public:
  Data();
  Data(char *data, int size, long long pts = 0) {
    this->data = new char[size];
    memcpy(this->data, data, size);
    this->size = size;
    this->pts = pts;  // record the time when storing the data
  };
  virtual ~Data() {
    if (this->data)
      delete this->data;
  };
  void Release() {
    if (this->data)
      delete this->data;
  }
  // int GetSize() const {
  //   return this->size;
  // };
  // int GetPts() const {
  //   return this->pts;
  // }
  // char GetData() const {
  //   return *(this->data);
  // }
  int size;
  long long pts = 0;
  char *data;

  //  private:
  // int size;
  // long long pts = 0;
  // char *data;
};

// camera 和 qt_audio 將擷取到的影像與聲音的資料傳到queue，之後需要封裝成packet時再pop出來
class DataManager : public QThread {
 public:
  DataManager(){};
  ~DataManager(){};
  // Get queue size
  int GetQueueSize();
  // Push data to the queue
  virtual void Push(Data &data);
  // Get the front data of queue, and pop it fron the queue
  virtual Data Pop();
  // Start the thread
  virtual void Start();
  // Stop the thread
  virtual void Stop();

  long long GetCurTime();

 protected:
  std::mutex mutex;
  std::queue<Data> data_queue;
  int data_count = 0;
  int max_queue_size = 100;
  bool is_exit = false;
};

#endif  // _DATA_MANAGER_H_