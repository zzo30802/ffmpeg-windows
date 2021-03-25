#include "DataManager.h"

void DataManager::Push(Data &data) {
  std::lock_guard<std::mutex> lock(mutex);
  // 檢查queue長度是否超過限制
  if (data_queue.size() > max_queue_size) {
    data_queue.front().Release();  // 手動釋放
    data_queue.pop();              // pop the front element
  }
  // 加入新資料
  data_queue.push(data);
}

Data DataManager::Pop() {
  std::lock_guard<std::mutex> lock(mutex);
  if (data_queue.empty())
    return Data();  // 回傳空Data
  Data &data = data_queue.front();
  data_queue.front().Release();
  data_queue.pop();
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