/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "MediaRecorder.h"

MediaRecorder::MediaRecorder(const char *url, RecorderParam *param) {
    LOGCATE("MediaRecorder::MediaRecorder url=%s", url);
    strcpy(m_OutUrl, url);
    m_RecorderParam = *param;
}

MediaRecorder::~MediaRecorder() {

}

int MediaRecorder::StartRecord() {
    LOGCATE("MediaRecorder::StartRecord");
    int result = 0;
    do {
        /* allocate the output media context */
        avformat_alloc_output_context2(&m_FormatCtx, NULL, NULL, m_OutUrl);
        if (!m_FormatCtx) {
            LOGCATE("MediaRecorder::StartRecord Could not deduce output format from file extension: using MPEG.\n");
            avformat_alloc_output_context2(&m_FormatCtx, NULL, "mpeg", m_OutUrl);
        }
        if (!m_FormatCtx) {
            result = -1;
            break;
        }

        m_OutputFormat = m_FormatCtx->oformat;

        /* Add the audio and video streams using the default format codecs
         * and initialize the codecs. */
        if (m_OutputFormat->video_codec != AV_CODEC_ID_NONE) {
            AddStream(&m_VideoStream, m_FormatCtx, &m_VideoCodec, m_OutputFormat->video_codec);
            m_EnableVideo = 1;
        }
        if (m_OutputFormat->audio_codec != AV_CODEC_ID_NONE) {
            AddStream(&m_AudioStream, m_FormatCtx, &m_AudioCodec, m_OutputFormat->audio_codec);
            m_EnableAudio = 1;
        }

        /* Now that all the parameters are set, we can open the audio and
         * video codecs and allocate the necessary encode buffers. */
        if (m_EnableVideo)
            OpenVideo(m_FormatCtx, m_VideoCodec, &m_VideoStream);

        if (m_EnableAudio)
            OpenAudio(m_FormatCtx, m_AudioCodec, &m_AudioStream);

        av_dump_format(m_FormatCtx, 0, m_OutUrl, 1);

        /* open the output file, if needed */
        if (!(m_OutputFormat->flags & AVFMT_NOFILE)) {
            int ret = avio_open(&m_FormatCtx->pb, m_OutUrl, AVIO_FLAG_WRITE);
            if (ret < 0) {
                LOGCATE("MediaRecorder::StartRecord Could not open '%s': %s", m_OutUrl,
                        av_err2str(ret));
                result = -1;
                break;
            }
        }

        /* Write the stream header, if any. */
        result = avformat_write_header(m_FormatCtx, nullptr);
        if (result < 0) {
            LOGCATE("MediaRecorder::StartRecord Error occurred when opening output file: %s",
                    av_err2str(result));
            result = -1;
            break;
        }

    } while (false);

    if (result >= 0) {
//        if(m_EnableAudio)
//            m_pAudioThread = new thread(StartAudioEncodeThread, this);
//        if(m_EnableVideo)
//            m_pVideoThread = new thread(StartVideoEncodeThread, this);
          if(m_pMediaThread == nullptr)
              m_pMediaThread = new thread(StartMediaEncodeThread, this);
    }

    return result;
}

int MediaRecorder::OnFrame2Encode(AudioFrame *inputFrame) {
    LOGCATE("MediaRecorder::OnFrame2Encode inputFrame->data=%p, inputFrame->dataSize=%d", inputFrame->data, inputFrame->dataSize);
    if(m_Exit) return 0;
    AudioFrame *pAudioFrame = new AudioFrame(inputFrame->data, inputFrame->dataSize);
    m_AudioFrameQueue.Push(pAudioFrame);
    return 0;
}

int MediaRecorder::OnFrame2Encode(VideoFrame *inputFrame) {
    if(m_Exit) return 0;
    LOGCATE("MediaRecorder::OnFrame2Encode [w,h,format]=[%d,%d,%d]", inputFrame->width, inputFrame->height, inputFrame->format);
    VideoFrame *pImage = new VideoFrame();
    pImage->width = inputFrame->width;
    pImage->height = inputFrame->height;
    pImage->format = inputFrame->format;
    NativeImageUtil::AllocNativeImage(pImage);
    NativeImageUtil::CopyNativeImage(inputFrame, pImage);
    m_VideoFrameQueue.Push(pImage);
    return 0;
}

int MediaRecorder::StopRecord() {
    LOGCATE("MediaRecorder::StopRecord");
    m_Exit = true;
    if(m_pAudioThread != nullptr || m_pVideoThread != nullptr || m_pMediaThread != nullptr) {

        if(m_pAudioThread != nullptr) {
            m_pAudioThread->join();
            delete m_pAudioThread;
            m_pAudioThread = nullptr;
        }

        if(m_pVideoThread != nullptr) {
            m_pVideoThread->join();
            delete m_pVideoThread;
            m_pVideoThread = nullptr;
        }

        if(m_pMediaThread != nullptr) {
            m_pMediaThread->join();
            delete m_pMediaThread;
            m_pMediaThread = nullptr;
        }

        while (!m_VideoFrameQueue.Empty()) {
            VideoFrame *pImage = m_VideoFrameQueue.Pop();
            NativeImageUtil::FreeNativeImage(pImage);
            delete pImage;
        }

        while (!m_AudioFrameQueue.Empty()) {
            AudioFrame *pAudio = m_AudioFrameQueue.Pop();
            delete pAudio;
        }

        int ret = av_write_trailer(m_FormatCtx);
        LOGCATE("MediaRecorder::StopRecord while av_write_trailer %s",
                av_err2str(ret));

        /* Close each codec. */
        if (m_EnableVideo)
            CloseStream(&m_VideoStream);
        if (m_EnableAudio)
            CloseStream(&m_AudioStream);

        if (!(m_OutputFormat->flags & AVFMT_NOFILE))
            /* Close the output file. */
            avio_closep(&m_FormatCtx->pb);

        /* free the stream */
        avformat_free_context(m_FormatCtx);
    }
    return 0;
}

void MediaRecorder::StartAudioEncodeThread(MediaRecorder *recorder) {
    LOGCATE("MediaRecorder::StartAudioEncodeThread start");
    AVOutputStream *vOs = &recorder->m_VideoStream;
    AVOutputStream *aOs = &recorder->m_AudioStream;
    AVFormatContext  *fmtCtx = recorder->m_FormatCtx;
    while (!aOs->m_EncodeEnd) {
        double videoTimestamp = vOs->m_NextPts * av_q2d(vOs->m_pCodecCtx->time_base);
        double audioTimestamp = aOs->m_NextPts * av_q2d(aOs->m_pCodecCtx->time_base);
        LOGCATE("MediaRecorder::StartAudioEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]", videoTimestamp, audioTimestamp);
        if (av_compare_ts(vOs->m_NextPts, vOs->m_pCodecCtx->time_base,
                                           aOs->m_NextPts, aOs->m_pCodecCtx->time_base) >= 0 || vOs->m_EncodeEnd) {
            LOGCATE("MediaRecorder::StartAudioEncodeThread start queueSize=%d", recorder->m_AudioFrameQueue.Size());
            //if(audioTimestamp >= videoTimestamp && vOs->m_EncodeEnd) aOs->m_EncodeEnd = vOs->m_EncodeEnd;
            aOs->m_EncodeEnd = recorder->EncodeAudioFrame(aOs);
        } else {
            LOGCATE("MediaRecorder::StartAudioEncodeThread usleep");
            usleep(5 * 1000);
        }
    }
    LOGCATE("MediaRecorder::StartAudioEncodeThread end");
}
void MediaRecorder::StartVideoEncodeThread(MediaRecorder *recorder) {
    LOGCATE("MediaRecorder::StartVideoEncodeThread start");
    AVOutputStream *vOs = &recorder->m_VideoStream;
    AVOutputStream *aOs = &recorder->m_AudioStream;
    while (!vOs->m_EncodeEnd) {
        double videoTimestamp = vOs->m_NextPts * av_q2d(vOs->m_pCodecCtx->time_base);
        double audioTimestamp = aOs->m_NextPts * av_q2d(aOs->m_pCodecCtx->time_base);
        LOGCATE("MediaRecorder::StartVideoEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]", videoTimestamp, audioTimestamp);
        if (av_compare_ts(vOs->m_NextPts, vOs->m_pCodecCtx->time_base,
                                           aOs->m_NextPts, aOs->m_pCodecCtx->time_base) <= 0 || aOs->m_EncodeEnd) {
            LOGCATE("MediaRecorder::StartVideoEncodeThread start queueSize=%d", recorder->m_VideoFrameQueue.Size());
            //视频和音频时间戳对齐，人对于声音比较敏感，防止出现视频声音播放结束画面还没结束的情况
            if(audioTimestamp <= videoTimestamp && aOs->m_EncodeEnd) vOs->m_EncodeEnd = aOs->m_EncodeEnd;
            vOs->m_EncodeEnd = recorder->EncodeVideoFrame(vOs);
        } else {
            LOGCATE("MediaRecorder::StartVideoEncodeThread start usleep");
            usleep(5 * 1000);
        }
    }
    LOGCATE("MediaRecorder::StartVideoEncodeThread end");
}


int MediaRecorder::WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                               AVPacket *pkt) {
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    PrintfPacket(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

void MediaRecorder::AddStream(AVOutputStream *ost, AVFormatContext *oc, AVCodec **codec,
                              AVCodecID codec_id) {
    LOGCATE("MediaRecorder::AddStream");
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        LOGCATE("MediaRecorder::AddStream Could not find encoder for '%s'",
                avcodec_get_name(codec_id));
        return;
    }

    ost->m_pStream = avformat_new_stream(oc, NULL);
    if (!ost->m_pStream) {
        LOGCATE("MediaRecorder::AddStream Could not allocate stream");
        return;
    }
    ost->m_pStream->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGCATE("MediaRecorder::AddStream Could not alloc an encoding context");
        return;
    }
    ost->m_pCodecCtx = c;

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            LOGCATE("MediaRecorder::AddStream AVMEDIA_TYPE_AUDIO AVCodecID=%d", codec_id);
            c->sample_fmt  = (*codec)->sample_fmts ?
                             (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            c->bit_rate    = 96000;
            c->sample_rate = m_RecorderParam.audioSampleRate;
            c->channel_layout = m_RecorderParam.channelLayout;
            c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
            ost->m_pStream->time_base = (AVRational){ 1, c->sample_rate };
            break;

        case AVMEDIA_TYPE_VIDEO:
            LOGCATE("MediaRecorder::AddStream AVMEDIA_TYPE_VIDEO AVCodecID=%d", codec_id);

            c->codec_id = codec_id;
            c->bit_rate = m_RecorderParam.videoBitRate;
            /* Resolution must be a multiple of two. */
            c->width    = m_RecorderParam.frameWidth;
            c->height   = m_RecorderParam.frameHeight;
            /* timebase: This is the fundamental unit of time (in seconds) in terms
             * of which frame timestamps are represented. For fixed-fps content,
             * timebase should be 1/framerate and timestamp increments should be
             * identical to 1. */
            ost->m_pStream->time_base = (AVRational){ 1, m_RecorderParam.fps };
            c->time_base       = ost->m_pStream->time_base;

            c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
            c->pix_fmt       = AV_PIX_FMT_YUV420P;
            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                /* just for testing, we also add B-frames */
                c->max_b_frames = 2;
            }
            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                /* Needed to avoid using macroblocks in which some coeffs overflow.
                 * This does not happen with normal video, it just happens here as
                 * the motion of the chroma plane does not match the luma plane. */
                c->mb_decision = 2;
            }
            break;

        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void MediaRecorder::PrintfPacket(AVFormatContext *fmt_ctx, AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGCATE("MediaRecorder::PrintfPacket pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

AVFrame *MediaRecorder::AllocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout,
                                        int sample_rate, int nb_samples) {
    LOGCATE("MediaRecorder::AllocAudioFrame");
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio frame");
        return nullptr;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio buffer");
            return nullptr;
        }
    }

    return frame;
}

int MediaRecorder::OpenAudio(AVFormatContext *oc, AVCodec *codec, AVOutputStream *ost) {
    LOGCATE("MediaRecorder::OpenAudio");
    AVCodecContext *c;
    int nb_samples;
    int ret;
    c = ost->m_pCodecCtx;
    /* open it */
    ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not open audio codec: %s", av_err2str(ret));
        return -1;
    }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->m_pFrame  = AllocAudioFrame(c->sample_fmt, c->channel_layout,
                                     c->sample_rate, nb_samples);

    ost->m_pTmpFrame = av_frame_alloc();

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->m_pStream->codecpar, c);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not copy the stream parameters");
        return -1;
    }

    /* create resampler context */
    ost->m_pSwrCtx = swr_alloc();
    if (!ost->m_pSwrCtx) {
        LOGCATE("MediaRecorder::OpenAudio Could not allocate resampler context");
        return -1;
    }

    /* set options */
    av_opt_set_int       (ost->m_pSwrCtx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->m_pSwrCtx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->m_pSwrCtx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->m_pSwrCtx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->m_pSwrCtx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->m_pSwrCtx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->m_pSwrCtx)) < 0) {
        LOGCATE("MediaRecorder::OpenAudio Failed to initialize the resampling context");
        return -1;
    }
    return 0;
}

int MediaRecorder::EncodeAudioFrame(AVOutputStream *ost) {
    LOGCATE("MediaRecorder::EncodeAudioFrame");
    int result = 0;
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->m_pCodecCtx;

    while (m_AudioFrameQueue.Empty() && !m_Exit) {
        usleep(10* 1000);
   }

    AudioFrame *audioFrame = m_AudioFrameQueue.Pop();
    frame = ost->m_pTmpFrame;
    if(audioFrame) {
        frame->data[0] = audioFrame->data;
        frame->nb_samples = audioFrame->dataSize / 4;
        frame->pts = ost->m_NextPts;
        ost->m_NextPts  += frame->nb_samples;
    }

    if((m_AudioFrameQueue.Empty() && m_Exit) || ost->m_EncodeEnd) frame = nullptr;

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->m_pSwrCtx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->m_pFrame);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while av_frame_make_writable");
            result = 1;
            goto EXIT;
        }

        /* convert to destination format */
        ret = swr_convert(ost->m_pSwrCtx,
                          ost->m_pFrame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        LOGCATE("MediaRecorder::EncodeAudioFrame dst_nb_samples=%d, frame->nb_samples=%d", dst_nb_samples, frame->nb_samples);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while converting");
            result = 1;
            goto EXIT;
        }
        frame = ost->m_pFrame;

        frame->pts = av_rescale_q(ost->m_SamplesCount, (AVRational){1, c->sample_rate}, c->time_base);
        ost->m_SamplesCount += dst_nb_samples;
    }

    ret = avcodec_send_frame(c, frame);
    if(ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if(ret < 0) {
        LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_send_frame fail. ret=%s", av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while(!ret) {
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_receive_packet fail. ret=%s", av_err2str(ret));
            result = 0;
            goto EXIT;
        }
        LOGCATE("MediaRecorder::EncodeAudioFrame pkt pts=%ld, size=%d", pkt.pts, pkt.size);
        int result = WritePacket(m_FormatCtx, &c->time_base, ost->m_pStream, &pkt);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }

EXIT:
    if(audioFrame) delete audioFrame;
    return result;
}

AVFrame *MediaRecorder::AllocVideoFrame(AVPixelFormat pix_fmt, int width, int height) {
    LOGCATE("MediaRecorder::AllocVideoFrame");
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return nullptr;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 1);
    if (ret < 0) {
        LOGCATE("MediaRecorder::AllocVideoFrame Could not allocate frame data.");
        return nullptr;
    }

    return picture;
}

int MediaRecorder::OpenVideo(AVFormatContext *oc, AVCodec *codec, AVOutputStream *ost) {
    LOGCATE("MediaRecorder::OpenVideo");
    int ret;
    AVCodecContext *c = ost->m_pCodecCtx;

    /* open the codec */
    ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not open video codec: %s", av_err2str(ret));
        return -1;
    }

    /* allocate and init a re-usable frame */
    ost->m_pFrame = AllocVideoFrame(c->pix_fmt, c->width, c->height);
    ost->m_pTmpFrame = av_frame_alloc();
    if (!ost->m_pFrame) {
        LOGCATE("MediaRecorder::OpenVideo Could not allocate video frame");
        return -1;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->m_pStream->codecpar, c);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not copy the stream parameters");
        return -1;
    }
    return 0;
}

int MediaRecorder::EncodeVideoFrame(AVOutputStream *ost) {
    LOGCATE("MediaRecorder::EncodeVideoFrame");
    int result = 0;
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    AVPacket pkt = { 0 };

    c = ost->m_pCodecCtx;

    av_init_packet(&pkt);

    while (m_VideoFrameQueue.Empty() && !m_Exit) {

        usleep(10* 1000);
    }

    frame = ost->m_pTmpFrame;
    AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
    VideoFrame *videoFrame = m_VideoFrameQueue.Pop();
    if(videoFrame) {
        frame->data[0] = videoFrame->ppPlane[0];
        frame->data[1] = videoFrame->ppPlane[1];
        frame->data[2] = videoFrame->ppPlane[2];
        frame->linesize[0] = videoFrame->pLineSize[0];
        frame->linesize[1] = videoFrame->pLineSize[1];
        frame->linesize[2] = videoFrame->pLineSize[2];
        frame->width = videoFrame->width;
        frame->height = videoFrame->height;
        switch (videoFrame->format) {
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
                LOGCATE("MediaRecorder::EncodeVideoFrame unSupport format pImage->format=%d", videoFrame->format);
                break;
        }
    }

    if((m_VideoFrameQueue.Empty() && m_Exit) || ost->m_EncodeEnd) frame = nullptr;

    if(frame != nullptr) {
    /* when we pass a frame to the encoder, it may keep a reference to it
    * internally; make sure we do not overwrite it here */
        if (av_frame_make_writable(ost->m_pFrame) < 0) {
            result = 1;
            goto EXIT;
        }

        if (srcPixFmt != AV_PIX_FMT_YUV420P) {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (!ost->m_pSwsCtx) {
                ost->m_pSwsCtx = sws_getContext(c->width, c->height,
                                                srcPixFmt,
                                              c->width, c->height,
                                              c->pix_fmt,
                                              SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
                if (!ost->m_pSwsCtx) {
                    LOGCATE("MediaRecorder::EncodeVideoFrame Could not initialize the conversion context\n");
                    result = 1;
                    goto EXIT;
                }
            }
            sws_scale(ost->m_pSwsCtx, (const uint8_t * const *) frame->data,
                      frame->linesize, 0, c->height, ost->m_pFrame->data,
                      ost->m_pFrame->linesize);
        }
        ost->m_pFrame->pts = ost->m_NextPts++;
        frame = ost->m_pFrame;
    }

//    if(frame) {
//        NativeImage i420;
//        i420.format = IMAGE_FORMAT_I420;
//        i420.width = frame->width;
//        i420.height = frame->height;
//        i420.ppPlane[0] = frame->data[0];
//        i420.ppPlane[1] = frame->data[1];
//        i420.ppPlane[2] = frame->data[2];
//        i420.pLineSize[0] = frame->linesize[0];
//        i420.pLineSize[1] = frame->linesize[1];
//        i420.pLineSize[2] = frame->linesize[2];
//        NativeImageUtil::DumpNativeImage(&i420, "/sdcard/DCIM", "media");
//    }

    /* encode the image */
    ret = avcodec_send_frame(c, frame);
    if(ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if(ret < 0) {
        LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_send_frame fail. ret=%s", av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while(!ret) {
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_receive_packet fail. ret=%s", av_err2str(ret));
            result = 0;
            goto EXIT;
        }
        LOGCATE("MediaRecorder::EncodeVideoFrame video pkt pts=%ld, size=%d", pkt.pts, pkt.size);
        int result = WritePacket(m_FormatCtx, &c->time_base, ost->m_pStream, &pkt);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }

EXIT:
    NativeImageUtil::FreeNativeImage(videoFrame);
    if(videoFrame) delete videoFrame;
    return result;
}

void MediaRecorder::CloseStream(AVOutputStream *ost) {
    avcodec_free_context(&ost->m_pCodecCtx);
    av_frame_free(&ost->m_pFrame);
    sws_freeContext(ost->m_pSwsCtx);
    swr_free(&ost->m_pSwrCtx);
    if(ost->m_pTmpFrame != nullptr) {
        av_free(ost->m_pTmpFrame);
        ost->m_pTmpFrame = nullptr;
    }

}

void MediaRecorder::StartMediaEncodeThread(MediaRecorder *recorder) {
    LOGCATE("MediaRecorder::StartMediaEncodeThread start");
    AVOutputStream *vOs = &recorder->m_VideoStream;
    AVOutputStream *aOs = &recorder->m_AudioStream;
    while (!vOs->m_EncodeEnd || !aOs->m_EncodeEnd) {
        double videoTimestamp = vOs->m_NextPts * av_q2d(vOs->m_pCodecCtx->time_base);
        double audioTimestamp = aOs->m_NextPts * av_q2d(aOs->m_pCodecCtx->time_base);
        LOGCATE("MediaRecorder::StartVideoEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]", videoTimestamp, audioTimestamp);
        if (!vOs->m_EncodeEnd &&
            (aOs->m_EncodeEnd || av_compare_ts(vOs->m_NextPts, vOs->m_pCodecCtx->time_base,
                                               aOs->m_NextPts, aOs->m_pCodecCtx->time_base) <= 0)) {
            //视频和音频时间戳对齐，人对于声音比较敏感，防止出现视频声音播放结束画面还没结束的情况
            if(audioTimestamp <= videoTimestamp && aOs->m_EncodeEnd) vOs->m_EncodeEnd = 1;
            vOs->m_EncodeEnd = recorder->EncodeVideoFrame(vOs);
        } else {
            aOs->m_EncodeEnd = recorder->EncodeAudioFrame(aOs);
        }
    }
    LOGCATE("MediaRecorder::StartVideoEncodeThread end");
}
