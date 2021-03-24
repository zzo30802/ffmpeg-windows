#include "data_manager.h"

class CVideoCapture : public VideoCaptureFactory {
 public:
  void run() {
    std::cout << "XXX 1" << std::endl;
  }
  bool Init(int camIndex = 0) {
    return true;
  }
  void Stop() {
  }
};

VideoCaptureFactory *VideoCaptureFactory::Get(unsigned char index) {
  static CVideoCapture instances[255];
  return &instances[index];
}