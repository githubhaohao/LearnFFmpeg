/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#ifndef LEARNFFMPEG_HWCODECPLAYER_H
#define LEARNFFMPEG_HWCODECPLAYER_H

#include <MediaPlayer.h>
#include <AVPacketQueue.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SyncClock.h>

#define BUFF_MAX_VIDEO_DURATION   0.5
#define MAX_SYNC_SLEEP_TIME       200
#define VIDEO_FRAME_DEFAULT_DELAY 25
#define VIDEO_FRAME_MAX_DELAY     250

#define AV_SYNC_THRESHOLD_MIN   40
#define AV_SYNC_THRESHOLD_MAX   100
#define AV_SYNC_FRAMEDUP_THRESHOLD AV_SYNC_THRESHOLD_MAX

enum PlayerState {
    PLAYER_STATE_UNKNOWN,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSE,
    PLAYER_STATE_STOP
};

class HWCodecPlayer : public MediaPlayer {
public:
    HWCodecPlayer(){};
    virtual ~HWCodecPlayer(){};

    virtual void Init(JNIEnv *jniEnv, jobject obj, char *url, int renderType, jobject surface);
    virtual void UnInit();

    virtual void Play();
    virtual void Pause();
    virtual void Stop();
    virtual void SeekToPosition(float position);
    virtual long GetMediaParams(int paramType);
    virtual void SetMediaParams(int paramType, jobject obj);

private:
    virtual JNIEnv *GetJNIEnv(bool *isAttach);
    virtual jobject GetJavaObj();
    virtual JavaVM *GetJavaVM();
    void AVSync();


    int InitDecoder();
    int DoMuxLoop();
    int UnInitDecoder();

    static void PostMessage(void *context, int msgType, float msgCode);
    static void DeMuxThreadProc(HWCodecPlayer* player);
    static void AudioDecodeThreadProc(HWCodecPlayer* player);
    static void VideoDecodeThreadProc(HWCodecPlayer* player);

private:
    AVPacketQueue*    m_VideoPacketQueue = nullptr;
    AVPacketQueue*    m_AudioPacketQueue = nullptr;
    AVFormatContext*   m_AVFormatContext = nullptr;
    char                 m_Url[MAX_PATH] = {0};
    AMediaCodec*            m_MediaCodec = nullptr;
    AMediaExtractor*    m_MediaExtractor = nullptr;
    AVBitStreamFilterContext*     m_Bsfc = nullptr;
    AVCodecContext*      m_AudioCodecCtx = nullptr;
    AVCodecContext*      m_VideoCodecCtx = nullptr;
    AVRational           m_VideoTimeBase = {0};
    AVRational           m_AudioTimeBase = {0};
    SwrContext*                 m_SwrCtx = nullptr;
    ANativeWindow*       m_ANativeWindow = nullptr;
    long                      m_Duration = 0;

    thread*              m_DeMuxThread   = nullptr;
    thread*              m_ADecodeThread = nullptr;
    thread*              m_VDecodeThread = nullptr;
    jobject              m_AssetMgr      = nullptr;

    PlayerState            m_PlayerState = PLAYER_STATE_UNKNOWN;
    volatile float        m_SeekPosition = -1;
    volatile bool          m_SeekSuccess = false;
    int                 m_VideoStreamIdx = -1;
    int                 m_AudioStreamIdx = -1;
    double              m_VideoStartBase = -1.0;
    double              m_AudioStartBase = -1.0;
    double             m_CommonStartBase = -1.0;


    //锁和条件变量
    mutex               m_Mutex;
    mutex               m_VideoBufMutex;
    mutex               m_AudioBufMutex;
    condition_variable  m_Cond;
    SyncClock           m_VideoClock;
    SyncClock           m_AudioClock;
    AVRational          m_FrameRate;
};


#endif //LEARNFFMPEG_HWCODECPLAYER_H
