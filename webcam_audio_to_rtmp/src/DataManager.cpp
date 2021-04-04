#include "DataManager.h"

#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

// bool ErrorMesssage(const int &error_num) {
//   char buf[1024]{0};
//   // put error code to buf
//   av_strerror(error_num, buf, sizeof(buf));
//   std::cout << buf << std::endl;
//   return false;
// }

//*********Data*********
Data::Data(char *src_data, int size, long long pts) {
  this->data = new char[size];  // new an array of char
  memcpy(this->data, src_data, size);
  this->size = size;
  this->pts = pts;
}
void Data::Release() {
  if (this->data)
    delete this->data;
  data = 0;
  size = 0;
}

//*********DataManager*********
void DataManager::Push(Data src_data) {
  std::lock_guard<std::mutex> lock(mutex);
  if (data_queue.size() > max_queue_size) {
    data_queue.front().Release();
    data_queue.pop_front();
  }
  data_queue.emplace_back(src_data);
}

Data DataManager::Pop() {
  std::lock_guard<std::mutex> lock(mutex);
  if (data_queue.empty())
    return Data();
  Data data = data_queue.front();
  data_queue.pop_front();
  return data;
}

// include QThread::start()
void DataManager::Start() {
  is_exit = false;
  QThread::start();
}

void DataManager::Stop() {
  is_exit = true;
  // Waiting for the thread to finish and then process the following code
  QThread::wait();
}