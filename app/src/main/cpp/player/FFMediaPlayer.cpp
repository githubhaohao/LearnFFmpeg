/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <render/video/NativeRender.h>
#include <render/audio/OpenSLRender.h>
#include <render/video/VideoGLRender.h>
#include <render/video/VRGLRender.h>
#include "FFMediaPlayer.h"

void FFMediaPlayer::Init(JNIEnv *jniEnv, jobject obj, char *url, int videoRenderType, jobject surface) {
    jniEnv->GetJavaVM(&m_JavaVM);
    m_JavaObj = jniEnv->NewGlobalRef(obj);

    m_VideoDecoder = new VideoDecoder(url);
    m_AudioDecoder = new AudioDecoder(url);

    if(videoRenderType == VIDEO_RENDER_OPENGL) {
        m_VideoDecoder->SetVideoRender(VideoGLRender::GetInstance());
    } else if (videoRenderType == VIDEO_RENDER_ANWINDOW) {
        m_VideoRender = new NativeRender(jniEnv, surface);
        m_VideoDecoder->SetVideoRender(m_VideoRender);
    } else if (videoRenderType == VIDEO_RENDER_3D_VR) {
        m_VideoDecoder->SetVideoRender(VRGLRender::GetInstance());
    }

    m_AudioRender = new OpenSLRender();
    m_AudioDecoder->SetAudioRender(m_AudioRender);

    m_VideoDecoder->SetMessageCallback(this, PostMessage);
    m_AudioDecoder->SetMessageCallback(this, PostMessage);
}

void FFMediaPlayer::UnInit() {
    LOGCATE("FFMediaPlayer::UnInit");
    if(m_VideoDecoder) {
        delete m_VideoDecoder;
        m_VideoDecoder = nullptr;
    }

    if(m_VideoRender) {
        delete m_VideoRender;
        m_VideoRender = nullptr;
    }

    if(m_AudioDecoder) {
        delete m_AudioDecoder;
        m_AudioDecoder = nullptr;
    }

    if(m_AudioRender) {
        delete m_AudioRender;
        m_AudioRender = nullptr;
    }

    VideoGLRender::ReleaseInstance();

    bool isAttach = false;
    GetJNIEnv(&isAttach)->DeleteGlobalRef(m_JavaObj);
    if(isAttach)
        GetJavaVM()->DetachCurrentThread();

}

void FFMediaPlayer::Play() {
    LOGCATE("FFMediaPlayer::Play");
    if(m_VideoDecoder)
        m_VideoDecoder->Start();

    if(m_AudioDecoder)
        m_AudioDecoder->Start();
}

void FFMediaPlayer::Pause() {
    LOGCATE("FFMediaPlayer::Pause");
    if(m_VideoDecoder)
        m_VideoDecoder->Pause();

    if(m_AudioDecoder)
        m_AudioDecoder->Pause();

}

void FFMediaPlayer::Stop() {
    LOGCATE("FFMediaPlayer::Stop");
    if(m_VideoDecoder)
        m_VideoDecoder->Stop();

    if(m_AudioDecoder)
        m_AudioDecoder->Stop();
}

void FFMediaPlayer::SeekToPosition(float position) {
    LOGCATE("FFMediaPlayer::SeekToPosition position=%f", position);
    if(m_VideoDecoder)
        m_VideoDecoder->SeekToPosition(position);

    if(m_AudioDecoder)
        m_AudioDecoder->SeekToPosition(position);

}

long FFMediaPlayer::GetMediaParams(int paramType) {
    LOGCATE("FFMediaPlayer::GetMediaParams paramType=%d", paramType);
    long value = 0;
    switch(paramType)
    {
        case MEDIA_PARAM_VIDEO_WIDTH:
            value = m_VideoDecoder != nullptr ? m_VideoDecoder->GetVideoWidth() : 0;
            break;
        case MEDIA_PARAM_VIDEO_HEIGHT:
            value = m_VideoDecoder != nullptr ? m_VideoDecoder->GetVideoHeight() : 0;
            break;
        case MEDIA_PARAM_VIDEO_DURATION:
            value = m_VideoDecoder != nullptr ? m_VideoDecoder->GetDuration() : 0;
            break;
    }
    return value;
}

JNIEnv *FFMediaPlayer::GetJNIEnv(bool *isAttach) {
    JNIEnv *env;
    int status;
    if (nullptr == m_JavaVM) {
        LOGCATE("FFMediaPlayer::GetJNIEnv m_JavaVM == nullptr");
        return nullptr;
    }
    *isAttach = false;
    status = m_JavaVM->GetEnv((void **)&env, JNI_VERSION_1_4);
    if (status != JNI_OK) {
        status = m_JavaVM->AttachCurrentThread(&env, nullptr);
        if (status != JNI_OK) {
            LOGCATE("FFMediaPlayer::GetJNIEnv failed to attach current thread");
            return nullptr;
        }
        *isAttach = true;
    }
    return env;
}

jobject FFMediaPlayer::GetJavaObj() {
    return m_JavaObj;
}

JavaVM *FFMediaPlayer::GetJavaVM() {
    return m_JavaVM;
}

void FFMediaPlayer::PostMessage(void *context, int msgType, float msgCode) {
    if(context != nullptr)
    {
        FFMediaPlayer *player = static_cast<FFMediaPlayer *>(context);
        bool isAttach = false;
        JNIEnv *env = player->GetJNIEnv(&isAttach);
        LOGCATE("FFMediaPlayer::PostMessage env=%p", env);
        if(env == nullptr)
            return;
        jobject javaObj = player->GetJavaObj();
        jmethodID mid = env->GetMethodID(env->GetObjectClass(javaObj), JAVA_PLAYER_EVENT_CALLBACK_API_NAME, "(IF)V");
        env->CallVoidMethod(javaObj, mid, msgType, msgCode);
        if(isAttach)
            player->GetJavaVM()->DetachCurrentThread();

    }
}


