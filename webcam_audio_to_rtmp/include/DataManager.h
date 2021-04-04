#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_
#include <QtCore/QThread>
#include <iostream>

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

static long long GetCurTime() {
  return av_gettime();
}

static bool ErrorMesssage(const int &error_num) {
  char buf[1024]{0};
  // put error code to buf
  av_strerror(error_num, buf, sizeof(buf));
  std::cout << buf << std::endl;
  return false;
}

class Data {
 public:
  Data(){};
  Data(char *src_data, int size, long long pts = 0);
  virtual ~Data(){};
  void Release();
  char *data = 0;
  int size = 0;
  long long pts = 0;
};

// To store data from a web camera or microphone
class DataManager : public QThread {
 public:
  DataManager(){};
  virtual ~DataManager(){};
  virtual void Push(Data src_data);
  virtual Data Pop();
  virtual void Start();
  virtual void Stop();

 protected:
  std::mutex mutex;
  int max_queue_size = 100;
  // int data_count = 0;
  std::list<Data> data_queue;
  bool is_exit = false;
};

#endif  // _DATA_MANAGER_H_