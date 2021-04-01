#ifndef _AUDIO_RECORD_FACTORY_H_
#define _AUDIO_RECORD_FACTORY_H_
#include "DataManager.h"
class AudioRecordFactory : public DataManager {
 public:
  // Qt connect to device && start recording
  virtual bool Init() = 0;
  // Stop recording
  virtual void Stop() = 0;
  static AudioRecordFactory *Get(unsigned char index = 0);
  ~AudioRecordFactory(){};

 protected:
  AudioRecordFactory(){};
  //*************Audio device config*************
  // Number of channel
  int channels = 2;
  // Sampling rating
  int sample_rate = 44100;
  int sample_byte = 2;
  // Number of sampling
  int nb_samples = 1024;
  //*********************************************
};
#endif  // _AUDIO_RECORD_FACTORY_H_