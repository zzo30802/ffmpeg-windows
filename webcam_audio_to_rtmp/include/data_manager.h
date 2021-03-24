#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_
#include <QtCore/QThread>
#include <iostream>
#include <list>
#include <mutex>

class DataManager : public QThread {
 public:
 protected:
  std::mutex mutex;
};

class VideoCaptureFactory : public DataManager {
 public:
  static VideoCaptureFactory *Get(unsigned char index = 0);
  virtual bool Init(int camIndex = 0) = 0;
  virtual void Stop() = 0;

 protected:
  VideoCaptureFactory();
};

#endif _DATA_MANAGER_H_