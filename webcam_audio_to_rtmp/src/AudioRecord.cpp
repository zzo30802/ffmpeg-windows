#include "AudioRecord.h"

#include <QtMultimedia/QAudioInput>
#include <iostream>
class AudioRecord : public AudioRecordFactory {
 public:
  void run() {
    int read_size{this->nb_sample * this->channels * this->sample_byte};
    char *buf = new char[read_size];
    while (!is_exit) {
      // Geat data from audio device
      if (input->bytesReady() < read_size) {
        QThread::msleep(1);
        continue;
      }
      int size = 0;
      while (size != read_size) {
        int &&len = io->read(buf + size, read_size - size);
        if (len < 0)
          break;
        size += len;
      }
      if (size != read_size) {
        continue;
      }
      // Push data of a frame to list
      Push(Data(buf, read_size, GetCurTime()));
    }
    delete buf;
  }

  // set parameter for audio device
  bool Init() {
    Stop();
    QAudioFormat fmt;
    fmt.setSampleRate(this->sample_rate);
    fmt.setChannelCount(this->channels);
    fmt.setSampleSize(this->sample_byte * 8);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(fmt)) {
      std::cout << "Audio format not support!" << std::endl;
      fmt = info.nearestFormat(fmt);
    }
    input = new QAudioInput(fmt);
    // Begin recording
    io = input->start();
    if (!io)
      return false;
    return true;
  }

  void Stop() {
    DataManager::Stop();  // QThread::wait();
    if (input)
      input->stop();
    if (io)
      io->close();
    input = nullptr;
    io = nullptr;
  }

  QAudioInput *input = nullptr;
  QIODevice *io = nullptr;
};

bool AudioRecordFactory::SetAudioDeviceParameter(enum class AudioDeviceParam param, const int &value) {
  // check value
  if (value < 0) {
    std::cout << "int value must be positive" << std::endl;
    return false;
  }

  switch (param) {
    case AudioDeviceParam::channels: {
      this->channels = value;
      return true;
    }
    case AudioDeviceParam::sample_rate: {
      this->sample_rate = value;
      return true;
    }
    case AudioDeviceParam::sample_byte: {
      this->sample_byte = value;
      return true;
    }
    case AudioDeviceParam::nb_sample: {
      this->nb_sample = value;
      return true;
    }
    default: {
      std::cout << "Error: bool AudioRecordFactory::SetAudioDeviceParameter()" << std::endl;
      std::cout << "  -> param is out of enum." << std::endl;
      return false;
    }
  }
  return false;
}

AudioRecordFactory *AudioRecordFactory::Get(unsigned char index) {
  static AudioRecord a_instance[255];
  return &a_instance[index];
}