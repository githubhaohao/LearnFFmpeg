/**
  ____        _             _____ _
 | __ ) _   _| |_ ___      |  ___| | _____      __
 |  _ \| | | | __/ _ \_____| |_  | |/ _ \ \ /\ / /
 | |_) | |_| | ||  __/_____|  _| | | (_) \ V  V /
 |____/ \__, |\__\___|     |_|   |_|\___/ \_/\_/
        |___/
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_VIDEODECODER_H
#define LEARNFFMPEG_VIDEODECODER_H

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/jni.h>
};

#include <render/video/VideoRender.h>
#include <SingleVideoRecorder.h>
#include "DecoderBase.h"

class VideoDecoder : public DecoderBase {

public:
    VideoDecoder(char *url){
        Init(url, AVMEDIA_TYPE_VIDEO);
    }

    virtual ~VideoDecoder(){
        UnInit();
    }

    int GetVideoWidth()
    {
        return m_VideoWidth;
    }
    int GetVideoHeight()
    {
        return m_VideoHeight;
    }

    void SetVideoRender(VideoRender *videoRender)
    {
        m_VideoRender = videoRender;
    }

private:
    virtual void OnDecoderReady();
    virtual void OnDecoderDone();
    virtual void OnFrameAvailable(AVFrame *frame);

    const AVPixelFormat DST_PIXEL_FORMAT = AV_PIX_FMT_RGBA;

    int m_VideoWidth = 0;
    int m_VideoHeight = 0;

    int m_RenderWidth = 0;
    int m_RenderHeight = 0;

    AVFrame *m_RGBAFrame = nullptr;
    uint8_t *m_FrameBuffer = nullptr;

    VideoRender *m_VideoRender = nullptr;
    SwsContext *m_SwsContext = nullptr;
    SingleVideoRecorder *m_pVideoRecorder = nullptr;
};


#endif //LEARNFFMPEG_VIDEODECODER_H
