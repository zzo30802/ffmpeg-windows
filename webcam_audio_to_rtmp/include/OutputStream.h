#ifndef _OUTPUT_FACTORY_H_
#define _OUTPUT_FACTORY_H_

#include "DataManager.h"

class OutputStream {
 public:
  ~OutputStream();  // Release
  // Singleton
  static OutputStream *Get();
  bool InitOutputAVFormatContext(const char *url);
  bool AddAVPacketToOutputAVFormatContext();

 private:
  static OutputStream *instance;
  AVFormatContext *output_av_format_context;
};
#endif  // _OUTPUT_FACTORY_H_