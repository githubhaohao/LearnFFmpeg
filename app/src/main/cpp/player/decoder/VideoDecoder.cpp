//
// Created by 字节流动 on 2020/6/17.
//

#include "VideoDecoder.h"

void VideoDecoder::OnDecoderReady() {
    LOGCATE("VideoDecoder::OnDecoderReady");
    m_VideoWidth = GetCodecContext()->width;
    m_VideoHeight = GetCodecContext()->height;

    if(m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_DECODER_READY, 0);

    if(m_VideoRender != nullptr) {
        int dstSize[2] = {0};
        m_VideoRender->Init(m_VideoWidth, m_VideoHeight, dstSize);
        m_RenderWidth = dstSize[0];
        m_RenderHeight = dstSize[1];

        m_RGBAFrame = av_frame_alloc();
        int bufferSize = av_image_get_buffer_size(DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight, 1);
        m_FrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(m_RGBAFrame->data, m_RGBAFrame->linesize,
                             m_FrameBuffer, DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight, 1);

        m_SwsContext = sws_getContext(m_VideoWidth, m_VideoHeight, GetCodecContext()->pix_fmt,
                                   m_RenderWidth, m_RenderHeight, DST_PIXEL_FORMAT,
                                   SWS_FAST_BILINEAR, NULL, NULL, NULL);

    } else {
        LOGCATE("VideoDecoder::OnDecoderReady m_VideoRender == null");
    }
}

void VideoDecoder::OnDecoderDone() {
    LOGCATE("VideoDecoder::OnDecoderDone");

    if(m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_DECODER_DONE, 0);

    if(m_VideoRender)
        m_VideoRender->UnInit();

    if(m_RGBAFrame != nullptr) {
        av_frame_free(&m_RGBAFrame);
        m_RGBAFrame = nullptr;
    }

    if(m_FrameBuffer != nullptr) {
        free(m_FrameBuffer);
        m_FrameBuffer = nullptr;
    }

    if(m_SwsContext != nullptr) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }

}

void VideoDecoder::OnFrameAvailable(AVFrame *frame) {
    LOGCATE("VideoDecoder::OnFrameAvailable frame=%p", frame);
    if(m_VideoRender != nullptr) {
        sws_scale(m_SwsContext, frame->data, frame->linesize, 0,
                  m_VideoHeight, m_RGBAFrame->data, m_RGBAFrame->linesize);
        NativeImage image;
        image.format = IMAGE_FORMAT_RGBA;
        image.width = m_RenderWidth;
        image.height = m_RenderHeight;
        image.ppPlane[0] = m_RGBAFrame->data[0];
        m_VideoRender->RenderVideoFrame(&image);
    }

    if(m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_REQUEST_RENDER, 0);
}

long VideoDecoder::GetVideoDecoderTimestampForAVSync(void *context) {
    if(context != nullptr)
    {
        VideoDecoder* videoDecoder = static_cast<VideoDecoder *>(context);
        return videoDecoder->GetCurrentPosition();
    }
    return 0;
}
