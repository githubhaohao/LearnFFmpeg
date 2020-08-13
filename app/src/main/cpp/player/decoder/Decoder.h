//
// Created by byteflow on 2020/6/17.
//

#ifndef LEARNFFMPEG_DECODER_H
#define LEARNFFMPEG_DECODER_H

typedef void (*MessageCallback)(void*, int, float);
typedef long (*AVSyncCallback)(void*);

class Decoder {
public:
    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual float GetDuration() = 0;
    virtual void SeekToPosition(float position) = 0;
    virtual float GetCurrentPosition() = 0;
    virtual void SetMessageCallback(void* context, MessageCallback callback) = 0;
};

#endif //LEARNFFMPEG_DECODER_H
