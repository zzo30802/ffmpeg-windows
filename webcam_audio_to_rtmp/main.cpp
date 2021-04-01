#include <QtCore/QCoreApplication>

// Data
#include "DataManager.h"
// Camera capture
#include "VideoCaptureFactory.h"
// Audio resumple
#include "AudioRecordFactory.h"
// ffmpeg : to encode Data from camera || microphone
#include "InputStream.h"   // video / audio encode
#include "OutputStream.h"  // video / audio decode

static const char *url = "rtmp://127.0.0.1:1935/live/home";

int main(int argc, char *argv[]) {
  std::cout << "main() start" << std::endl;
  QCoreApplication a(argc, argv);

  //============Open all device==============
  // get camera instance
  VideoCaptureFactory *video_capture_instance = VideoCaptureFactory::Get(0);
  if (!video_capture_instance->Init(0)) {  // open webcamera and set param
    return -1;
  }
  video_capture_instance->Start();  // DataManager::Start() -> QThread start -> run() start
  std::cout << "Open Webcamera success && start to capture image!" << std::endl;

  // get microphone instance
  AudioRecordFactory *audio_resample_instance = AudioRecordFactory::Get(0);
  if (!audio_resample_instance->Init()) {
    return -1;
  }
  audio_resample_instance->Start();
  std::cout << "Open microphone success && start to record " << std::endl;

  //============Get Data from queue && encode Data==============
  // video
  // init video struct and set parameters
  VideoStreamingContext *video_struct = new VideoStreamingContext();
  std::cout << video_struct->fps << std::endl;
  std::cout << "XXX 0" << std::endl;
  VideoInput *input_video_process = VideoInput::Get(video_struct);  // Get instance for VideoInput
  std::cout << "XXX 0-1" << std::endl;
  if (!input_video_process->InitContext(AVPixelFormat::AV_PIX_FMT_BGR24, AVPixelFormat::AV_PIX_FMT_YUV420P)) return -1;
  std::cout << "XXX 0-2" << std::endl;
  if (!input_video_process->InitAndOpenAVCodecContext(AVCodecID::AV_CODEC_ID_H264)) return -1;
  std::cout << "XXX 0-3" << std::endl;
  if (!input_video_process->InitAndGetAVFrameFromData()) return -1;

  std::cout << "XXX 1" << std::endl;

  // audio
  // init audio struct and set parameters
  AudioStreamingContext *audio_struct = new AudioStreamingContext();
  std::cout << "XXX 1-1" << std::endl;
  AudioInput *input_audio_process = AudioInput::Get(audio_struct);  // Get instance for AudioInput
  std::cout << "XXX 1-2" << std::endl;
  if (!input_audio_process->InitContext(AVSampleFormat::AV_SAMPLE_FMT_FLTP, AVSampleFormat::AV_SAMPLE_FMT_S16)) return -1;
  std::cout << "XXX 1-3" << std::endl;
  if (!input_audio_process->InitAndOpenAVCodecContext(AVCodecID::AV_CODEC_ID_AAC)) return -1;
  std::cout << "XXX 1-4" << std::endl;
  if (!input_audio_process->InitAndGetAVFrameFromData()) return -1;

  std::cout << "XXX 2" << std::endl;
  /*
  //============Get encoded data and add then to AVStream==============
  and then conbine two of AVStream(video/audio) into a AVFormatContext. Finally, upstreaming.*/
  OutputStream *rtmp_streaming = OutputStream::Get();
  if (!rtmp_streaming->InitOutputAVFormatContext(url)) return -1;
  // add video AVSteam channel to AVFormatContext
  int &&video_stream_index = rtmp_streaming->InitAVCodecContextAndAVStream(video_struct->av_codec_context);
  if (video_stream_index == -1) return -1;
  // add audio AVStream channel to AVFormatContext
  int &&audio_stream_index = rtmp_streaming->InitAVCodecContextAndAVStream(audio_struct->av_codec_context);
  if (audio_stream_index == -1) return -1;
  if (!rtmp_streaming->OpenOutputURL()) {
    video_capture_instance->Stop();
    audio_resample_instance->Stop();
    return -1;
  }

  std::cout << "XXX 3" << std::endl;

  //============Get Data from queue and encode them==============
  long long begin_time = DataManager::GetCurTime();  // get initial time_base
  while (true) {
    std::cout << "XXX 4" << std::endl;
    Data video_data = video_capture_instance->Pop();
    std::cout << "XXX 4-1" << std::endl;
    Data audio_data = audio_resample_instance->Pop();
    std::cout << "XXX 4-2" << std::endl;
    // check input data
    if (video_data.size <= 0 || audio_data.size <= 0) {
      std::cout << "XXX 5" << std::endl;
      QThread::msleep(1);
      continue;  // if lose one of the data between video or audio, skip this loop.
    }
    std::cout << "XXX 6" << std::endl;
    //-----Audio processing-----
    if (audio_data.size > 0) {
      audio_data.pts = audio_data.pts - begin_time;
      // resample
      Data pcm = input_audio_process->Resample(audio_data);
      audio_data.Release();
      Data encoded_pcm = input_audio_process->EncodeAudio(pcm);
      pcm.Release();
      if (encoded_pcm.size > 0) {
        if (rtmp_streaming->AddAVStreamToAVFormatContext(encoded_pcm, audio_stream_index)) {
          std::cout << "#" << std::flush;
        }
      }
    }
    std::cout << "XXX 11" << std::endl;
    //-----Video processing-----
    if (video_data.size > 0) {
      video_data.pts = video_data.pts - begin_time;
      // convert pixel format : RGB -> YUV
      Data yuv = input_video_process->ConvertPixelFromat(video_data);
      std::cout << "XXX 7" << std::endl;
      video_data.Release();
      std::cout << "XXX 8" << std::endl;
      Data encoded_yuv = input_video_process->EncodeVideo(yuv);
      std::cout << "XXX 9" << std::endl;
      yuv.Release();
      std::cout << "XXX 10" << std::endl;
      if (encoded_yuv.size > 0) {
        if (rtmp_streaming->AddAVStreamToAVFormatContext(encoded_yuv, video_stream_index)) {
          std::cout << "@" << std::flush;
        }
      }
    }
  }

  delete video_capture_instance;
  delete audio_resample_instance;
  std::cout << "main() end" << std::endl;
  return a.exec();
}