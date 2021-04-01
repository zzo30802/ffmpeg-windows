#include "AudioRecordFactory.h"

#include <QtMultimedia/QAudioInput>
// https://github.com/hurtnotbad/QTFFmpegAudioRtmp/blob/master/QTFFmpegAudioRtmp/main.cpp
class AudioRecord : public AudioRecordFactory {
 public:
  bool Init() {
    Stop();
    // Config QAudioFormat
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
    // new memory to QAudioInput
    input = new QAudioInput(fmt);
    // Begin recording
    io = input->start();
    if (!io)
      return false;
    return true;
  }

  void run() {
    std::cout << "AudioRecord::run() start" << std::endl;
    // read a frame data
    int readSize{this->nb_samples * this->channels * this->sample_byte};
    char *buf = new char[readSize];
    while (!is_exit) {
      // get a frame of data at a time
      if (input->bytesReady() < readSize) {
        QThread::msleep(1);
        continue;
      }
      int size = 0;
      while (size != readSize) {
        int len = io->read(buf + size, readSize - size);
        if (len < 0)
          break;
        size += len;
      }
      if (size != readSize)
        continue;
      Push(Data(buf, readSize, GetCurTime()));
    }
    std::cout << "AudioRecord::run() end" << std::endl;
    delete buf;
  }

  void Stop() {
    DataManager::Stop();
    if (input)
      input->stop();
    if (io)
      io->close();
    input = nullptr;
    io = nullptr;
  }

 public:
  //*******QT Audio IO object***********
  QAudioInput *input = nullptr;
  QIODevice *io = nullptr;
  //************************************
};

AudioRecordFactory *AudioRecordFactory::Get(unsigned char index) {
  static AudioRecord instance[255];
  return &instance[index];
}