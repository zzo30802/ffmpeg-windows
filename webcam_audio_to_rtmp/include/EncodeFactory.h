#ifndef _ENCODE_FACTORY_H_
#define _ENCODE_FACTORY_H_

class EncodeFacotry {
 public:
  //**************Video***************
  virtual bool InitVideoContext();
  //**********************************

  //**************Audio***************
  virtual bool InitAudioContext();
  //**********************************
};

#endif  // _ENCODE_FACTORY_H_