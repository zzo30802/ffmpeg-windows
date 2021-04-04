#ifndef _VIDEO_CAPTURE_H_
#define _VIDEO_CAPTURE_H_

#include "DataManager.h"

enum class CameraDeviceParam {
  width = 0,
  height,
  fps,
};

class VideoCaptureFactory : public DataManager {
 public:
  // Get instance by singleton pattern
  static VideoCaptureFactory *Get(unsigned char index = 0);
  // set camera index to open
  virtual bool Init(int cam_index = 0) = 0;
  // Stop capture
  virtual void Stop() = 0;
  // int GetCameraDeviceParameter(enum class CameraDeviceParam param) const;

  virtual ~VideoCaptureFactory(){};

  //******camera setting******
  int width = 0;
  int height = 0;
  int fps = 0;

 protected:
  VideoCaptureFactory(){};
  // //******camera setting******
  // int width = 0;
  // int height = 0;
  // int fps = 0;
};

#endif  // _VIDEO_CAPTURE_H_