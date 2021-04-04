#include <QtCore/QCoreApplication>
#include <iostream>

// Data
#include "DataManager.h"

// connect camera && microphone
#include "AudioRecord.h"
#include "VideoCapture.h"

// Input stream
#include "InputStream.h"

// Output stream
#include "OutputStream.h"

extern "C" {
#include <libavformat/avformat.h>
}
using namespace std;
static const char *url = "rtmp://127.0.0.1:1935/live/home";

int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);

  //*****Audio device config
  int &&channels = 2;
  int &&sample_rate = 44100;
  int &&sample_byte = 2;
  int &&nb_sample = 1024;

  std::cout << "XXX 1" << std::endl;

  //*****Open Camera to capture
  VideoCaptureFactory *camera_instance = VideoCaptureFactory::Get(0);
  if (!camera_instance->Init(0))
    return -1;
  // int &&frame_width = camera_instance->GetCameraDeviceParameter(CameraDeviceParam::width);
  // int &&frame_height = camera_instance->GetCameraDeviceParameter(CameraDeviceParam::height);
  // int &&frame_fps = camera_instance->GetCameraDeviceParameter(CameraDeviceParam::fps);
  // std::cout << "frame_width : " << frame_width << std::endl;
  // std::cout << "frame_height : " << frame_height << std::endl;
  // std::cout << "frame_fps : " << frame_fps << std::endl;

  camera_instance->Start();
  std::cout << "XXX 2" << std::endl;

  //*****Open microphone to reocrd
  AudioRecordFactory *audio_instance = AudioRecordFactory::Get(0);
  audio_instance->SetAudioDeviceParameter(AudioDeviceParam::channels, channels);
  audio_instance->SetAudioDeviceParameter(AudioDeviceParam::sample_rate, sample_rate);
  audio_instance->SetAudioDeviceParameter(AudioDeviceParam::sample_byte, sample_byte);
  audio_instance->SetAudioDeviceParameter(AudioDeviceParam::nb_sample, nb_sample);
  if (!audio_instance->Init())
    return -1;
  audio_instance->Start();
  std::cout << "XXX 3" << std::endl;

  //*****Video stream input
  VideoInputFactory *video_input_instance = VideoInputFactory::Get(0);
  video_input_instance->fps = camera_instance->fps;
  // set input frame size
  video_input_instance->src_img_width = camera_instance->width;
  video_input_instance->src_img_height = camera_instance->height;
  // set output frame size
  video_input_instance->dst_img_width = camera_instance->width;
  video_input_instance->dst_img_height = camera_instance->height;

  if (!video_input_instance->InitContext(AVPixelFormat::AV_PIX_FMT_BGR24, AVPixelFormat::AV_PIX_FMT_YUV420P)) return -1;
  if (!video_input_instance->InitAndOpenAVCodecContext(AVCodecID::AV_CODEC_ID_H264)) return -1;
  if (!video_input_instance->InitAVFrame()) return -1;

  //*****Audio stream input
  AudioInputFactory *audio_input_instance = AudioInputFactory::Get(0);
  audio_input_instance->channels = channels;
  audio_input_instance->sample_rate = sample_rate;
  audio_input_instance->sample_byte = sample_byte;
  audio_input_instance->nb_sample = nb_sample;
  if (!audio_input_instance->InitContext(AVSampleFormat::AV_SAMPLE_FMT_FLTP, AVSampleFormat::AV_SAMPLE_FMT_S16)) return -1;
  if (!audio_input_instance->InitAndOpenAVCodecContext(AVCodecID::AV_CODEC_ID_AAC)) return -1;
  if (!audio_input_instance->InitAVFrame()) return -1;

  //*****Output Stream
  OutputStream *rtmp_streaming = OutputStream::Get();
  if (!rtmp_streaming->InitOutputAVFormatContext(url)) return -1;
  int &&video_stream_index = rtmp_streaming->InitAVCodecContextAndAVStream(video_input_instance->av_codec_context);
  if (video_stream_index < 0) return -1;
  int &&audio_stream_index = rtmp_streaming->InitAVCodecContextAndAVStream(audio_input_instance->av_codec_context);
  if (audio_stream_index < 0) return -1;
  if (!rtmp_streaming->OpenOutputURL()) {
    camera_instance->Stop();
    audio_instance->Stop();
    return -1;
  }

  //*****Get Data from queue and encode the
  long long begin_time = GetCurTime();  // get initial time_base
  while (true) {
    Data video_data = camera_instance->Pop();
    Data audio_data = audio_instance->Pop();
    if (video_data.size <= 0 && audio_data.size <= 0) {
      QThread::msleep(1);
      continue;
    }
    //-----Video processing-----
    if (video_data.size > 0) {
      video_data.pts = video_data.pts - begin_time;
      Data yuv = video_input_instance->ConvertPixelFormat(video_data);
      std::cout << "yuv.size : " << yuv.size << std::endl;
      video_data.Release();
      Data encoded_yuv = video_input_instance->EncodeVideo(yuv);
      std::cout << "encoded_yuv.size : " << encoded_yuv.size << std::endl;
      if (encoded_yuv.size > 0) {
        if (rtmp_streaming->AddAVStreamToAVFormatContext(encoded_yuv, video_stream_index))
          std::cout << "encoded_yuv.size : " << encoded_yuv.size << std::endl;
        // std::cout << "@" << std::flush;
      }
    }
    //-----Audio processing-----
    if (audio_data.size > 0) {
      audio_data.pts = audio_data.pts - begin_time;
      Data pcm = audio_input_instance->Resample(audio_data);
      audio_data.Release();
      Data encoded_pcm = audio_input_instance->EncodeAudio(pcm);
      if (encoded_pcm.size > 0) {
        if (rtmp_streaming->AddAVStreamToAVFormatContext(encoded_pcm, audio_stream_index))
          std::cout << "#" << std::flush;
      }
    }
  }
  return a.exec();
}