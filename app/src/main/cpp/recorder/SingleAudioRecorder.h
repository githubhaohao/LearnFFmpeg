//
// Created by 公众号：字节流动 on 2021/2/23.
//

#ifndef LEARNFFMPEG_SINGLEAUDIORECORDER_H
#define LEARNFFMPEG_SINGLEAUDIORECORDER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "ThreadSafeQueue.h"
#include "thread"
#include "LogUtil.h"
#include "AudioRender.h"

using namespace std;

#define DEFAULT_SAMPLE_RATE    44100
#define DEFAULT_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO

class SingleAudioRecorder {
public:
    SingleAudioRecorder(const char *outUrl, int sampleRate, int channelLayout, int sampleFormat);
    ~SingleAudioRecorder();

    int StartRecord();
    int OnFrame2Encode(AudioFrame *inputFrame);
    int StopRecord();

private:
    static void StartAACEncoderThread(SingleAudioRecorder *context);
    int EncodeFrame(AVFrame *pFrame);
private:
    ThreadSafeQueue<AudioFrame *> m_frameQueue;
    char m_outUrl[1024] = {0};
    int m_frameIndex = 0;
    int m_sampleRate;
    int m_channelLayout;
    int m_sampleFormat;
    AVPacket m_avPacket;
    AVFrame  *m_pFrame = nullptr;
    uint8_t *m_pFrameBuffer = nullptr;
    int m_frameBufferSize;
    AVCodec  *m_pCodec = nullptr;
    AVStream *m_pStream = nullptr;
    AVCodecContext *m_pCodecCtx = nullptr;
    AVFormatContext *m_pFormatCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;
    thread *m_encodeThread = nullptr;
    volatile int m_exit = 0;
};


#endif //LEARNFFMPEG_SINGLEAUDIORECORDER_H
