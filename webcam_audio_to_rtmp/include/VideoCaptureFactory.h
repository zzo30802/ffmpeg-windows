#ifndef _VIDEO_CAPTURE_H_
#define _VIDEO_CAPTURE_H_
#include "DataManager.h"

// abstract class for video capture
class VideoCaptureFactory : public DataManager {
 public:
  ~VideoCaptureFactory(){};
  static VideoCaptureFactory *Get(unsigned char index = 0);

  virtual bool Init(const int &camIndex = 0) = 0;
  virtual void Stop() = 0;

 protected:
  VideoCaptureFactory(){};
  int width = 0;
  int height = 0;
  int fps = 0;
};

#endif  // _VIDEO_CAPTURE_H_