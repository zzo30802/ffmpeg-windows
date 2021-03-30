#include "OutputStream.h"

OutputStream* OutputStream::Get() {
  if (instance == nullptr)
    instance = new OutputStream();
  return instance;
}

bool OutputStream::InitOutputAVFormatContext(const char* url) {
  int ret = avformat_alloc_output_context2(&this->output_av_format_context, nullptr, "flv", url);
  if (!ret) {
    std::cout << "Error: VideoProcess::InitAVFormatContext()" << std::endl;
    return ErrorMesssage(ret);
  }
  return true;
}