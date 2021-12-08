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
        pts = 0;
        lastUpdate = 0;
    }

    ~SyncClock() {
    }

    void SetClock(double pts, double time) {
        this->pts = pts;
        this->lastUpdate = time;
    }

    double GetClock() {
        double time = GetSysCurrentTime();
        return pts + time - lastUpdate;
    }
private:
    double pts;
    double lastUpdate;
};


#endif //LEARNFFMPEG_SYNCCLOCK_H
