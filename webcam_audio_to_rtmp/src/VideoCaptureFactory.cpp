#include "VideoCaptureFactory.h"

#include <opencv2/highgui.hpp>
// Get() -> Init() -> Start() / run()
class VideoCapture : public VideoCaptureFactory {
 public:
  // DataManager中的Start()->QThread::Start()執行

  bool Init(const int &camIndex = 0) {
    std::cout << "VideoCapture::Init() start" << std::endl;
    cam.open(camIndex);
    if (!cam.isOpened()) {
      std::cout << "camera open failed." << std::endl;
      return false;
    }
    std::cout << "camera index : " << camIndex << ", open success." << std::endl;
    cam.set(CV_CAP_PROP_FPS, 25);
    this->width = cam.get(cv::CAP_PROP_FRAME_WIDTH);
    this->height = cam.get(cv::CAP_PROP_FRAME_HEIGHT);
    this->fps = cam.get(cv::CAP_PROP_FPS);
    if (this->fps == 0) this->fps = 25;
    std::cout << "VideoCapture::Init() end" << std::endl;
    return true;
  }

  void run() {  // start()
    std::cout << "VideoCapture::run() start" << std::endl;
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
      Data data((char *)frame.data, frame.cols * frame.cols * frame.elemSize(), GetCurTime());
      Push(data);
    }
    std::cout << "VideoCapture::run() end" << std::endl;
  }

  void Stop() {
    DataManager::Stop();
    if (cam.isOpened())
      cam.release();
  }

 private:
  cv::VideoCapture cam;
};

VideoCaptureFactory *VideoCaptureFactory::Get(unsigned char index) {
  static VideoCapture instances[255];
  return &instances[index];
}