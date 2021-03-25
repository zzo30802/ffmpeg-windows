#include <QtCore/QCoreApplication>

#include "data_manager.h"
int main(int argc, char *argv[]) {
  std::cout << "XXX 1" << std::endl;
  QCoreApplication a(argc, argv);
  VideoCaptureFactory *b = VideoCaptureFactory::Get(0);
  b->Init();
  std::cout << "XXX 2" << std::endl;
  return a.exec();
}