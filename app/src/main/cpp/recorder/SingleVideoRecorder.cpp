//
// Created by ByteFlow on 2021/2/22.
//

#include "SingleVideoRecorder.h"

SingleVideoRecorder::SingleVideoRecorder(const char *outUrl, int frameWidth, int frameHeight,
                                         long bitRate, int fps) {
    LOGCATE("SingleVideoRecorder::SingleVideoRecorder outUrl=%s, [w,h]=[%d,%d], bitRate=%ld, fps=%d", outUrl, frameWidth, frameHeight, bitRate, fps);
    strcpy(m_outUrl, outUrl);
    m_frameWidth = frameWidth;
    m_frameHeight = frameHeight;
    m_bitRate = bitRate;
    m_frameRate = fps;
}

SingleVideoRecorder::~SingleVideoRecorder() {

}

int SingleVideoRecorder::StartRecord() {
    LOGCATE("SingleVideoRecorder::StartRecord");
    int result = 0;
    do{
        result = avformat_alloc_output_context2(&m_pFormatCtx, nullptr, nullptr, m_outUrl);
        if(result < 0) {
            LOGCATE("SingleVideoRecorder::StartRecord avformat_alloc_output_context2 ret=%d", result);
            break;
        }

        result = avio_open(&m_pFormatCtx->pb, m_outUrl, AVIO_FLAG_READ_WRITE);
        if(result < 0) {
            LOGCATE("SingleVideoRecorder::StartRecord avio_open ret=%d", result);
            break;
        }

        m_pStream = avformat_new_stream(m_pFormatCtx, nullptr);
        if (m_pStream == nullptr) {
            result = -1;
            LOGCATE("SingleVideoRecorder::StartRecord avformat_new_stream fail. ret=%d", result);
            break;
        }

        m_pCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
        if (m_pCodec == nullptr) {
            result = -1;
            LOGCATE("SingleVideoRecorder::StartRecord avcodec_find_encoder fail. ret=%d", result);
            break;
        }

        m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
        m_pCodecCtx->codec_id = m_pCodec->id;
        m_pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        m_pCodecCtx->width = m_frameWidth;
        m_pCodecCtx->height = m_frameHeight;
        m_pCodecCtx->time_base.num = 1;
        m_pCodecCtx->time_base.den = m_frameRate;
        m_pCodecCtx->bit_rate = m_bitRate;
        m_pCodecCtx->gop_size = 24;

        result = avcodec_parameters_from_context(m_pStream->codecpar, m_pCodecCtx);
        if(result < 0) {
            LOGCATE("SingleVideoRecorder::StartRecord avcodec_parameters_from_context ret=%d", result);
            break;
        }

        av_stream_set_r_frame_rate(m_pStream, {1, m_frameRate});

        result = avcodec_open2(m_pCodecCtx, m_pCodec, nullptr);
        if(result < 0) {
            LOGCATE("SingleVideoRecorder::StartRecord avcodec_open2 ret=%d", result);
            break;
        }

        av_dump_format(m_pFormatCtx, 0, m_outUrl, 1);

        m_pFrame = av_frame_alloc();
        m_pFrame->width = m_pCodecCtx->width;
        m_pFrame->height = m_pCodecCtx->height;
        m_pFrame->format = m_pCodecCtx->pix_fmt;
        int bufferSize = av_image_get_buffer_size(m_pCodecCtx->pix_fmt, m_pCodecCtx->width,
                                                  m_pCodecCtx->height, 1);
        m_pFrameBuffer = (uint8_t *) av_malloc(bufferSize);
        av_image_fill_arrays(m_pFrame->data, m_pFrame->linesize, m_pFrameBuffer, m_pCodecCtx->pix_fmt,
                             m_pCodecCtx->width, m_pCodecCtx->height, 1);

        AVDictionary *opt = 0;
        if (m_pCodecCtx->codec_id == AV_CODEC_ID_H264) {
            av_dict_set_int(&opt, "video_track_timescale", 25, 0);
            av_dict_set(&opt, "preset", "slow", 0);
            av_dict_set(&opt, "tune", "zerolatency", 0);
        }
        avformat_write_header(m_pFormatCtx, &opt);
        av_new_packet(&m_avPacket, bufferSize * 3);

    } while(false);

    if(result >=0) {
        m_encodeThread = new thread(StartH264EncoderThread, this);
    }
    return result;
}

int SingleVideoRecorder::StopRecord() {
    m_exit = 1;
    if(m_encodeThread != nullptr) {
        m_encodeThread->join();
        delete m_encodeThread;
        m_encodeThread = nullptr;

        int result = EncodeFrame(m_pCodecCtx, nullptr, &m_avPacket);
        if(result >= 0) {
            av_write_trailer(m_pFormatCtx);
        }
    }

    while (!m_frameQueue.Empty()) {
        NativeImage *pImage = m_frameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
    }

    if (m_pCodecCtx != nullptr) {
        avcodec_close(m_pCodecCtx);
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
    if (m_pFrame != nullptr) {
        av_free(m_pFrame);
        m_pFrame = nullptr;
    }
    if (m_pFrameBuffer != nullptr) {
        av_free(m_pFrameBuffer);
        m_pFrameBuffer = nullptr;
    }
    if (m_pFormatCtx != nullptr) {
        avio_close(m_pFormatCtx->pb);
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }

    return 0;
}

void SingleVideoRecorder::StartH264EncoderThread(SingleVideoRecorder *recorder) {
    LOGCATE("SingleVideoRecorder::StartH264EncoderThread start");
    while (!recorder->m_exit)
    {
        if(recorder->m_frameQueue.Empty()) {
            usleep(10 * 1000);
            continue;
        }

        NativeImage *pImage = recorder->m_frameQueue.Pop();
        AVFrame *pFrame = recorder->m_pFrame;
        pFrame->data[0] = pImage->ppPlane[0];
        pFrame->data[1] = pImage->ppPlane[1];
        pFrame->data[2] = pImage->ppPlane[2];
        pFrame->pts = recorder->m_frameIndex++;
        recorder->EncodeFrame(recorder->m_pCodecCtx, pFrame, &recorder->m_avPacket);
        NativeImageUtil::FreeNativeImage(pImage);
    }

    LOGCATE("SingleVideoRecorder::StartH264EncoderThread end");
}

int SingleVideoRecorder::OnFrame2Encode(NativeImage *inputFrame) {
    if(m_exit) return 0;
    LOGCATE("SingleVideoRecorder::OnFrame2Encode [w,h,format]=[%d,%d,%d]", inputFrame->width, inputFrame->height, inputFrame->format);
    NativeImage *pImage = new NativeImage();
    pImage->width = inputFrame->width;
    pImage->height = inputFrame->height;
    pImage->format = inputFrame->format;
    NativeImageUtil::AllocNativeImage(pImage);
    NativeImageUtil::CopyNativeImage(inputFrame, pImage);
    m_frameQueue.Push(pImage);
    return 0;
}

int SingleVideoRecorder::EncodeFrame(AVCodecContext *pCodecCtx, AVFrame *pFrame, AVPacket *pPacket) {
    int result = 0;
    result = avcodec_send_frame(pCodecCtx, pFrame);
    if(result < 0)
    {
        LOGCATE("SingleVideoRecorder::EncodeFrame avcodec_send_frame fail. ret=%d", result);
        return result;
    }
    while(!result) {
        result = avcodec_receive_packet(pCodecCtx, pPacket);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return 0;
        } else if (result < 0) {
            LOGCATE("SingleVideoRecorder::EncodeFrame avcodec_receive_packet fail. ret=%d", result);
            return result;
        }
        LOGCATE("SingleVideoRecorder::EncodeFrame frame pts=%ld, size=%d", pPacket->pts, pPacket->size);
        pPacket->stream_index = m_pStream->index;
        av_packet_rescale_ts(pPacket, pCodecCtx->time_base, m_pStream->time_base);
        pPacket->pos = -1;
        av_interleaved_write_frame(m_pFormatCtx, pPacket);
        av_packet_unref(pPacket);
    }
    return 0;
}
