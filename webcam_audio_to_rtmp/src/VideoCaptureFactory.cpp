#include "VideoCaptureFactory.h"

#include <opencv2/highgui.hpp>
// Get() -> Init() -> Start() / run()
class VideoCapture : public VideoCaptureFactory {
 public:
  void run() {  // start()
    std::cout << "VideoCapture::run() start" << std::endl;
    cv::Mat frame;
    while (!is_exit) {
      if (!cam.read(frame)) {
        std::cout << "!cam.read(frame)" << std::endl;
        QThread::msleep(1);
        continue;
      }
      if (frame.empty()) {
        std::cout << "!frame.empty()" << std::endl;
        QThread::msleep(1);
        continue;
      }
      Data data((char *)frame.data, frame.cols * frame.rows * frame.elemSize(), GetCurTime());
      std::cout << "VideoCapture push 1" << std::endl;
      Push(data);
      std::cout << "VideoCapture push 2" << std::endl;
    }
    std::cout << "VideoCapture::run() end" << std::endl;
  }

  // DataManager中的Start()->QThread::Start()執行
  bool Init(int camIndex = 0) {
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