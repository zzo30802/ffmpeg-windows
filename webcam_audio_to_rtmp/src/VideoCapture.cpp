#include "VideoCapture.h"

#include <iostream>
#include <opencv2/highgui.hpp>

class VideoCapture : public VideoCaptureFactory {
 public:
  void run() {
    std::cout << "The camera starts to capture images" << std::endl;
    cv::Mat frame;
    while (!is_exit) {
      if (!cam.read(frame)) {
        QThread::msleep(1);
        continue;
      }
      if (frame.empty()) {
        QThread::msleep(1);
        continue;
      }
      Data data((char *)frame.data, frame.cols * frame.rows * frame.elemSize(), GetCurTime());
      Push(data);
    }
  }

  bool Init(int cam_index = 0) {
    this->cam.open(cam_index);
    if (!this->cam.isOpened()) {
      std::cout << "The camera [" << cam_index << "] open failed." << std::endl;
      return false;
    }
    std::cout << "The camera [" << cam_index << "] open success." << std::endl;
    this->cam.set(CV_CAP_PROP_FPS, 25);
    this->width = this->cam.get(cv::CAP_PROP_FRAME_WIDTH);
    this->height = this->cam.get(cv::CAP_PROP_FRAME_HEIGHT);
    this->fps = this->cam.get(cv::CAP_PROP_FPS);
    if (fps == 0) fps = 25;
    return true;
  }

  void Stop() {
    DataManager::Stop();
    if (this->cam.isOpened()) {
      this->cam.release();
    }
  }

 private:
  cv::VideoCapture cam;
};

// if fail will return -1.
// int VideoCaptureFactory::GetCameraDeviceParameter(enum class CameraDeviceParam param) const {
//   switch (param) {
//     case CameraDeviceParam::width: {
//       return this->width;
//     }
//     case CameraDeviceParam::height: {
//       return this->height;
//     }
//     case CameraDeviceParam::fps: {
//       return this->fps;
//     }
//     default: {
//       std::cout << "Error: int VideoCaptureFactory::GetCameraDeviceParameter()" << std::endl;
//       std::cout << "  -> (enum) param is not in enumeration" << std::endl;
//       return -1;
//     }
//   }
//   return -1;
// }

VideoCaptureFactory *VideoCaptureFactory::Get(unsigned char index) {
  static VideoCapture v_instance[255];
  return &v_instance[index];
}
