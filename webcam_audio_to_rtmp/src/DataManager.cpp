#include "DataManager.h"

Data::Data() {}
Data::~Data() {}
Data::Data(char* data, int size, long long pts) {
  this->data = new char[size];
  memcpy(this->data, data, size);
  this->size = size;
  this->pts = pts;
}
void Data::Release() {
  if (data)
    delete data;
  data = 0;
  size = 0;
}

void DataManager::Push(Data data) {
  std::lock_guard<std::mutex> lock(mutex);
  if (data_queue.size() > max_queue_size) {
    data_queue.front().Release();
    data_queue.pop_front();
  }
  // 加入新資料
  data_queue.push_back(data);
  // std::cout << "queue size : " << data_queue.size() << std::endl;
  // std::cout << data.size << std::endl;
}

Data DataManager::Pop() {
  std::lock_guard<std::mutex> lock(mutex);
  if (data_queue.empty()) {
    return Data();  // 回傳空Data
  }
  Data data = data_queue.front();
  data_queue.pop_front();
  return data;
}

void DataManager::Start() {
  is_exit = false;
  QThread::start();
}

void DataManager::Stop() {
  is_exit = true;
  QThread::wait();
}

long long DataManager::GetCurTime() {
  return av_gettime();
}