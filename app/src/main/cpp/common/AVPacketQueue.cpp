/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <LogUtil.h>
#include "AVPacketQueue.h"

AVPacketQueue::AVPacketQueue() {
    abort_request = 0;
    first_pkt = nullptr;
    last_pkt = nullptr;
    nb_packets = 0;
    size = 0;
    duration = 0;
}

AVPacketQueue::~AVPacketQueue() {
    Abort();
    Flush();
}

int AVPacketQueue::Put(AVPacket *pkt) {
    AVPacketNode *pkt1;

    if (abort_request) {
        return -1;
    }

    pkt1 = (AVPacketNode *) av_malloc(sizeof(AVPacketNode));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt = *pkt;
    pkt1->next = nullptr;

    if (!last_pkt) {
        first_pkt = pkt1;
    } else {
        last_pkt->next = pkt1;
    }
    last_pkt = pkt1;
    nb_packets++;
    size += pkt1->pkt.size + sizeof(*pkt1);
    duration += pkt1->pkt.duration;
    return 0;
}

int AVPacketQueue::PushPacket(AVPacket *pkt) {
    int ret;
    unique_lock<mutex> lock(m_Mutex);
    ret = Put(pkt);
    m_CondVar.notify_all();
    lock.unlock();

    if (ret < 0) {
        av_packet_unref(pkt);
    }

    return ret;
}

int AVPacketQueue::PushNullPacket(int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return PushPacket(pkt);
}

void AVPacketQueue::Flush() {
    AVPacketNode *pkt, *pkt1;
    unique_lock<mutex> lock(m_Mutex);
    for (pkt = first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    last_pkt = nullptr;
    first_pkt = nullptr;
    nb_packets = 0;
    size = 0;
    duration = 0;
    m_CondVar.notify_all();
}

void AVPacketQueue::Abort() {
    unique_lock<mutex> lock(m_Mutex);
    abort_request = 1;
    m_CondVar.notify_all();
}

void AVPacketQueue::Start() {
    unique_lock<mutex> lock(m_Mutex);
    abort_request = 0;
    m_CondVar.notify_all();
}

int AVPacketQueue::GetPacket(AVPacket *pkt) {
    return GetPacket(pkt, 1);
}

int AVPacketQueue::GetPacket(AVPacket *pkt, int block) {
    AVPacketNode *pkt1;
    int ret;
    unique_lock<mutex> lock(m_Mutex);
    for (;;) {
        if (abort_request) {
            ret = -1;
            break;
        }

        pkt1 = first_pkt;
        if (pkt1) {
            first_pkt = pkt1->next;
            if (!first_pkt) {
                last_pkt = nullptr;
            }
            nb_packets--;
            size -= pkt1->pkt.size + sizeof(*pkt1);
            duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            m_CondVar.wait(lock);
        }
    }
    return ret;
}

int AVPacketQueue::GetPacketSize() {
    unique_lock<mutex> lock(m_Mutex);
    return nb_packets;
}

int AVPacketQueue::GetSize() {
    unique_lock<mutex> lock(m_Mutex);
    return size;
}

int64_t AVPacketQueue::GetDuration() {
    unique_lock<mutex> lock(m_Mutex);
    return duration;
}

int AVPacketQueue::IsAbort() {
    unique_lock<mutex> lock(m_Mutex);
    return abort_request;
}