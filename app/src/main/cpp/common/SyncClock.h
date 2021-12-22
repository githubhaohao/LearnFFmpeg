/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_SYNCCLOCK_H
#define LEARNFFMPEG_SYNCCLOCK_H

#include <LogUtil.h>

class SyncClock {
public:
    SyncClock(){
        curPts = 0;
        lastUpdate = 0;
        lastPts = 0;
        frameTimer = 0;
    }

    ~SyncClock() {
    }

    void SetClock(double pts, double time) {
        this->curPts = pts;
        this->lastUpdate = time;
    }

    double GetClock() {
        double time = GetSysCurrentTime();
        return curPts + time - lastUpdate;
    }

public:
    double lastPts;
    double frameTimer;
    double curPts;

private:
    double lastUpdate;
};


#endif //LEARNFFMPEG_SYNCCLOCK_H
