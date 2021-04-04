#ifndef _AUDIO_RECORD_H_
#define _AUDIO_RECORD_H_

#include "DataManager.h"

enum class AudioDeviceParam {
  channels = 0,
  sample_rate,
  sample_byte,
  nb_sample
};

/**
 * Description : Open the microphone and start to record, and 
 *               then get data to push it into data_queue. 
**/
class AudioRecordFactory : public DataManager {
 public:
  // Get instance by singleton pattern
  static AudioRecordFactory *Get(unsigned char index = 0);
  // connect to device && open it
  virtual bool Init() = 0;
  //
  virtual void Stop() = 0;
  bool SetAudioDeviceParameter(enum class AudioDeviceParam param, const int &value);

  virtual ~AudioRecordFactory(){};

 protected:
  AudioRecordFactory(){};
  //*******audio device setting**********
  int &&channels = 2;
  int &&sample_rate = 44100;
  int &&sample_byte = 2;
  int &&nb_sample = 1024;
};

#endif  // _AUDIO_RECORD_H_