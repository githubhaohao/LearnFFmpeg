/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <unistd.h>
#include "SingleAudioRecorder.h"

SingleAudioRecorder::SingleAudioRecorder(const char *outUrl, int sampleRate, int channelLayout, int sampleFormat) {
    LOGCATE("SingleAudioRecorder::SingleAudioRecorder outUrl=%s, sampleRate=%d, channelLayout=%d, sampleFormat=%d", outUrl, sampleRate, channelLayout, sampleFormat);
    strcpy(m_outUrl, outUrl);
    m_sampleRate = sampleRate;
    m_channelLayout = channelLayout;
    m_sampleFormat = sampleFormat;
}

SingleAudioRecorder::~SingleAudioRecorder() {

}

int SingleAudioRecorder::StartRecord() {
    int result = -1;
    do {
        result = avformat_alloc_output_context2(&m_pFormatCtx, nullptr, nullptr, m_outUrl);
        if(result < 0) {
            LOGCATE("SingleAudioRecorder::StartRecord avformat_alloc_output_context2 ret=%d", result);
            break;
        }

        result = avio_open(&m_pFormatCtx->pb, m_outUrl, AVIO_FLAG_READ_WRITE);
        if(result < 0) {
            LOGCATE("SingleAudioRecorder::StartRecord avio_open ret=%d", result);
            break;
        }

        m_pStream = avformat_new_stream(m_pFormatCtx, nullptr);
        if (m_pStream == nullptr) {
            result = -1;
            LOGCATE("SingleAudioRecorder::StartRecord avformat_new_stream fail. ret=%d", result);
            break;
        }

        AVOutputFormat *avOutputFormat = m_pFormatCtx->oformat;
        m_pCodec = avcodec_find_encoder(avOutputFormat->audio_codec);
        if (m_pCodec == nullptr) {
            result = -1;
            LOGCATE("SingleAudioRecorder::StartRecord avcodec_find_encoder fail. ret=%d", result);
            break;
        }

        m_pCodecCtx = m_pStream->codec;
        m_pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
        LOGCATE("SingleAudioRecorder::StartRecord avOutputFormat->audio_codec=%d", avOutputFormat->audio_codec);
        m_pCodecCtx->codec_id = AV_CODEC_ID_AAC;
        m_pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
        m_pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;//float, planar, 4 字节
        m_pCodecCtx->sample_rate = DEFAULT_SAMPLE_RATE;
        m_pCodecCtx->channel_layout = DEFAULT_CHANNEL_LAYOUT;
        m_pCodecCtx->channels = av_get_channel_layout_nb_channels(m_pCodecCtx->channel_layout);
        m_pCodecCtx->bit_rate = 96000;
//        if (avOutputFormat->flags & AVFMT_GLOBALHEADER) {
//            m_pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//        }

        result = avcodec_open2(m_pCodecCtx, m_pCodec, nullptr);
        if(result < 0) {
            LOGCATE("SingleAudioRecorder::StartRecord avcodec_open2 ret=%d", result);
            break;
        }

        av_dump_format(m_pFormatCtx, 0, m_outUrl, 1);

        m_pFrame = av_frame_alloc();
        m_pFrame->nb_samples = m_pCodecCtx->frame_size;
        m_pFrame->format = m_pCodecCtx->sample_fmt;

        m_frameBufferSize = av_samples_get_buffer_size(nullptr, m_pCodecCtx->channels, m_pCodecCtx->frame_size,
                                                m_pCodecCtx->sample_fmt, 1);
        LOGCATE("SingleAudioRecorder::StartRecord m_frameBufferSize=%d, nb_samples=%d", m_frameBufferSize, m_pFrame->nb_samples);
        m_pFrameBuffer = (uint8_t *) av_malloc(m_frameBufferSize);
        avcodec_fill_audio_frame(m_pFrame, m_pCodecCtx->channels, m_pCodecCtx->sample_fmt,
                                 (const uint8_t *) m_pFrameBuffer, m_frameBufferSize, 1);

        //写文件头
        avformat_write_header(m_pFormatCtx, nullptr);
        av_new_packet(&m_avPacket, m_frameBufferSize);

        //音频转码器
        m_swrCtx = swr_alloc();
        av_opt_set_channel_layout(m_swrCtx,  "in_channel_layout", m_channelLayout, 0);
        av_opt_set_channel_layout(m_swrCtx,  "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_int(m_swrCtx,  "in_sample_rate", m_sampleRate, 0);
        av_opt_set_int(m_swrCtx,  "out_sample_rate", DEFAULT_SAMPLE_RATE, 0);
        av_opt_set_sample_fmt(m_swrCtx,  "in_sample_fmt", AVSampleFormat(m_sampleFormat), 0);
        av_opt_set_sample_fmt(m_swrCtx,  "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
        swr_init(m_swrCtx);

    } while (false);

    if(result >= 0) {
        m_encodeThread = new thread(StartAACEncoderThread, this);
    }

    return 0;
}

int SingleAudioRecorder::OnFrame2Encode(AudioFrame *inputFrame) {
    LOGCATE("SingleAudioRecorder::OnFrame2Encode nputFrame->data=%p, inputFrame->dataSize=%d", inputFrame->data, inputFrame->dataSize);
    if(m_exit) return 0;
    AudioFrame *pAudioFrame = new AudioFrame(inputFrame->data, inputFrame->dataSize);
    m_frameQueue.Push(pAudioFrame);
    return 0;
}

int SingleAudioRecorder::StopRecord() {
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
        AudioFrame *pAudioFrame = m_frameQueue.Pop();
        delete pAudioFrame;
    }

    if(m_swrCtx != nullptr) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
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
    LOGCATE("SingleAudioRecorder m_pFormatCtx=%p", m_pFormatCtx);
    if (m_pFormatCtx != nullptr) {
        avio_close(m_pFormatCtx->pb);
        //avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }
    return 0;
}

void SingleAudioRecorder::StartAACEncoderThread(SingleAudioRecorder *recorder) {
    LOGCATE("SingleAudioRecorder::StartAACEncoderThread start");
    while (!recorder->m_exit || !recorder->m_frameQueue.Empty())
    {
        if(recorder->m_frameQueue.Empty()) {
            //队列为空，休眠等待
            usleep(10 * 1000);
            continue;
        }

        AudioFrame *audioFrame = recorder->m_frameQueue.Pop();
        AVFrame *pFrame = recorder->m_pFrame;
        int result = swr_convert(recorder->m_swrCtx, pFrame->data, pFrame->nb_samples, (const uint8_t **) &(audioFrame->data), audioFrame->dataSize / 4);
        LOGCATE("SingleAudioRecorder::StartAACEncoderThread result=%d", result);
        if(result >= 0) {
            pFrame->pts = recorder->m_frameIndex++;
            recorder->EncodeFrame(pFrame);
        }
        delete audioFrame;
    }

    LOGCATE("SingleAudioRecorder::StartAACEncoderThread end");
}

int SingleAudioRecorder::EncodeFrame(AVFrame *pFrame) {
    LOGCATE("SingleAudioRecorder::EncodeFrame pFrame->nb_samples=%d", pFrame != nullptr ? pFrame->nb_samples : 0);
    int result = 0;
    result = avcodec_send_frame(m_pCodecCtx, pFrame);
    if(result < 0)
    {
        LOGCATE("SingleAudioRecorder::EncodeFrame avcodec_send_frame fail. ret=%d", result);
        return result;
    }
    while(!result) {
        result = avcodec_receive_packet(m_pCodecCtx, &m_avPacket);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return 0;
        } else if (result < 0) {
            LOGCATE("SingleAudioRecorder::EncodeFrame avcodec_receive_packet fail. ret=%d", result);
            return result;
        }
        LOGCATE("SingleAudioRecorder::EncodeFrame frame pts=%ld, size=%d", m_avPacket.pts, m_avPacket.size);
        m_avPacket.stream_index = m_pStream->index;
        av_interleaved_write_frame(m_pFormatCtx, &m_avPacket);
        av_packet_unref(&m_avPacket);
    }
    return 0;
}
