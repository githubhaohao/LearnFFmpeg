/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "PlayerWrapper.h"

void PlayerWrapper::Init(JNIEnv *jniEnv, jobject obj, char *url, int playerType, int renderType,
                         jobject surface) {
    switch (playerType) {
        case FFMEDIA_PLAYER:
            m_MediaPlayer = new FFMediaPlayer();
            break;
        case HWCODEC_PLAYER:
            m_MediaPlayer = new HWCodecPlayer();
            break;
        default:
            break;
    }

    if(m_MediaPlayer)
        m_MediaPlayer->Init(jniEnv, obj, url, renderType, surface);
}

void PlayerWrapper::UnInit() {
    if(m_MediaPlayer) {
        m_MediaPlayer->UnInit();
        delete m_MediaPlayer;
        m_MediaPlayer = nullptr;
    }
}

void PlayerWrapper::Play() {
    if(m_MediaPlayer) {
        m_MediaPlayer->Play();
    }
}

void PlayerWrapper::Pause() {
    if(m_MediaPlayer) {
        m_MediaPlayer->Pause();
    }
}

void PlayerWrapper::Stop() {
    if(m_MediaPlayer) {
        m_MediaPlayer->Stop();
    }
}

void PlayerWrapper::SeekToPosition(float position) {
    if(m_MediaPlayer) {
        m_MediaPlayer->SeekToPosition(position);
    }

}

long PlayerWrapper::GetMediaParams(int paramType) {
    if(m_MediaPlayer) {
        return m_MediaPlayer->GetMediaParams(paramType);
    }

    return 0;
}

void PlayerWrapper::SetMediaParams(int paramType, jobject obj) {
    if(m_MediaPlayer) {
        m_MediaPlayer->SetMediaParams(paramType, obj);
    }

}
