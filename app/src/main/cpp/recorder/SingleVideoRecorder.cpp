/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


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
        m_pCodecCtx->gop_size = 15;

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

        int result = EncodeFrame(nullptr);
        if(result >= 0) {
            av_write_trailer(m_pFormatCtx);
        }
    }

    while (!m_frameQueue.Empty()) {
        NativeImage *pImage = m_frameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        delete pImage;
    }

    if (m_pCodecCtx != nullptr) {
        avcodec_close(m_pCodecCtx);
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
    if (m_pFrame != nullptr) {
        av_frame_free(&m_pFrame);
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

    if(m_SwsContext != nullptr) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }

    return 0;
}

void SingleVideoRecorder::StartH264EncoderThread(SingleVideoRecorder *recorder) {
    LOGCATE("SingleVideoRecorder::StartH264EncoderThread start");
    //停止编码且队列为空时退出循环
    while (!recorder->m_exit || !recorder->m_frameQueue.Empty())
    {
        if(recorder->m_frameQueue.Empty()) {
            //队列为空，休眠等待
            usleep(10 * 1000);
            continue;
        }
        //从队列中取一帧预览帧
        NativeImage *pImage = recorder->m_frameQueue.Pop();
        AVFrame *pFrame = recorder->m_pFrame;
        AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
        switch (pImage->format) {
            case IMAGE_FORMAT_RGBA:
                srcPixFmt = AV_PIX_FMT_RGBA;
                break;
            case IMAGE_FORMAT_NV21:
                srcPixFmt = AV_PIX_FMT_NV21;
                break;
            case IMAGE_FORMAT_NV12:
                srcPixFmt = AV_PIX_FMT_NV12;
                break;
            case IMAGE_FORMAT_I420:
                srcPixFmt = AV_PIX_FMT_YUV420P;
                break;
            default:
                LOGCATE("SingleVideoRecorder::StartH264EncoderThread unsupport format pImage->format=%d", pImage->format);
                break;
        }
        if(srcPixFmt != AV_PIX_FMT_YUV420P) {
            if(recorder->m_SwsContext == nullptr) {
                recorder->m_SwsContext = sws_getContext(pImage->width, pImage->height, srcPixFmt,
                                                        recorder->m_frameWidth, recorder->m_frameHeight, AV_PIX_FMT_YUV420P,
                                                        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
            }
            //转换为编码器的目标格式 AV_PIX_FMT_YUV420P
            if(recorder->m_SwsContext != nullptr) {
                int slice = sws_scale(recorder->m_SwsContext, pImage->ppPlane, pImage->pLineSize, 0,
                          recorder->m_frameHeight, pFrame->data, pFrame->linesize);
//                NativeImage i420;
//                i420.format = IMAGE_FORMAT_I420;
//                i420.width = pFrame->width;
//                i420.height = pFrame->height;
//                i420.ppPlane[0] = pFrame->data[0];
//                i420.ppPlane[1] = pFrame->data[1];
//                i420.ppPlane[2] = pFrame->data[2];
//                i420.pLineSize[0] = pFrame->linesize[0];
//                i420.pLineSize[1] = pFrame->linesize[1];
//                i420.pLineSize[2] = pFrame->linesize[2];
//                NativeImageUtil::DumpNativeImage(&i420, "/sdcard/DCIM", "NDK");
                LOGCATE("SingleVideoRecorder::StartH264EncoderThread sws_scale slice=%d", slice);
            }
        }
        //设置 pts
        pFrame->pts = recorder->m_frameIndex++;
        recorder->EncodeFrame(pFrame);
        NativeImageUtil::FreeNativeImage(pImage);
        delete pImage;
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
    //NativeImageUtil::DumpNativeImage(pImage, "/sdcard", "camera");
    m_frameQueue.Push(pImage);
    return 0;
}

int SingleVideoRecorder::EncodeFrame(AVFrame *pFrame) {
    int result = 0;
    result = avcodec_send_frame(m_pCodecCtx, pFrame);
    if(result < 0)
    {
        LOGCATE("SingleVideoRecorder::EncodeFrame avcodec_send_frame fail. ret=%d", result);
        return result;
    }
    while(!result) {
        result = avcodec_receive_packet(m_pCodecCtx, &m_avPacket);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return 0;
        } else if (result < 0) {
            LOGCATE("SingleVideoRecorder::EncodeFrame avcodec_receive_packet fail. ret=%d", result);
            return result;
        }
        LOGCATE("SingleVideoRecorder::EncodeFrame frame pts=%ld, size=%d", m_avPacket.pts, m_avPacket.size);
        m_avPacket.stream_index = m_pStream->index;
        av_packet_rescale_ts(&m_avPacket, m_pCodecCtx->time_base, m_pStream->time_base);
        m_avPacket.pos = -1;
        av_interleaved_write_frame(m_pFormatCtx, &m_avPacket);
        av_packet_unref(&m_avPacket);
    }
    return 0;
}
