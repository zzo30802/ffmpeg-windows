#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_
#include <QtCore/QThread>
#include <iostream>
#include <mutex>
#include <queue>

extern "C" {
#include <libavutil/time.h>
}

struct Data {
 public:
  Data();
  Data(char *data, int size, long long pts = 0) {
    this->data = new char[size];
    memcpy(this->data, data, size);
    this->size = size;
    this->pts = pts;
  };
  virtual ~Data() {
    if (data)
      delete data;
  };
  void Release() {
    if (data)
      delete data;
  }
  int GetSize() {
    return size;
  };

 private:
  int size;
  long long pts = 0;
  char *data;
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