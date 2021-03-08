//
// Created by 公众号：字节流动 on 2021/3/5.
//

#ifndef LEARNFFMPEG_MEDIARECORDER_H
#define LEARNFFMPEG_MEDIARECORDER_H

#include <ImageDef.h>
#include <render/audio/AudioRender.h>
#include "ThreadSafeQueue.h"
#include "thread"

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

using namespace std;

typedef NativeImage VideoFrame;

class AVOutputStream {
public:
    AVOutputStream() {
        m_pStream = nullptr;
        m_pCodecCtx = nullptr;
        m_pFrame = nullptr;
        m_pTmpFrame = nullptr;
        m_pSwrCtx = nullptr;
        m_pSwsCtx = nullptr;
        m_NextPts = 0;
        m_SamplesCount = 0;
    }

    ~AVOutputStream(){}
public:
    AVStream *m_pStream;
    AVCodecContext *m_pCodecCtx;
    volatile int64_t m_NextPts;
    int m_SamplesCount;
    AVFrame *m_pFrame;
    AVFrame *m_pTmpFrame;
    SwsContext *m_pSwsCtx;
    SwrContext *m_pSwrCtx;
};

struct RecorderParam {
    //video
    int frameWidth;
    int frameHeight;
    long videoBitRate;
    int fps;

    //audio
    int audioSampleRate;
    int channelLayout;
    int sampleFormat;
};

class MediaRecorder {
public:
    MediaRecorder(const char *url, RecorderParam *param);
    ~MediaRecorder();

    int StartRecord();
    int OnFrame2Encode(AudioFrame *inputFrame);
    int OnFrame2Encode(VideoFrame *inputFrame);
    int StopRecord();

private:
    static void StartAudioEncodeThread(MediaRecorder *recorder);

    static void StartVideoEncodeThread(MediaRecorder *recorder);

    AVFrame *AllocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);

    AVFrame *AllocVideoFrame(AVPixelFormat pix_fmt, int width, int height);

    int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st, AVPacket *pkt);

    void AddStream(AVOutputStream *ost, AVFormatContext *oc, AVCodec **codec, AVCodecID codec_id);

    void PrintfPacket(AVFormatContext *fmt_ctx, AVPacket *pkt);

    int OpenAudio(AVFormatContext *oc, AVCodec *codec, AVOutputStream *ost, AVDictionary *opt_arg);

    int OpenVideo(AVFormatContext *oc, AVCodec *codec, AVOutputStream *ost, AVDictionary *opt_arg);

    int WriteAudioFrame(AVFormatContext *oc, AVOutputStream *ost);

    int WriteVideoFrame(AVFormatContext *oc, AVOutputStream *ost);

    void CloseStream(AVFormatContext *oc, AVOutputStream *ost);

private:
    RecorderParam    m_RecorderParam = {0};
    AVOutputStream   m_VideoStream;
    AVOutputStream   m_AudioStream;
    char             m_OutUrl[1024] = {0};
    AVOutputFormat  *m_OutputFormat = nullptr;
    AVFormatContext *m_FormatCtx = nullptr;
    AVCodec         *m_AudioCodec = nullptr;
    AVCodec         *m_VideoCodec = nullptr;
    ThreadSafeQueue<VideoFrame *>
                     m_VideoFrameQueue;
    ThreadSafeQueue<AudioFrame *>
                     m_AudioFrameQueue;
    int              m_EnableVideo = 0;
    int              m_EnableAudio = 0;
    volatile bool    m_Exit = false;
    thread          *m_pAudioThread = nullptr;
    thread          *m_pVideoThread = nullptr;

};


#endif //LEARNFFMPEG_MEDIARECORDER_H
