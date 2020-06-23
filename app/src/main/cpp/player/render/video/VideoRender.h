//
// Created by 字节流动 on 2020/6/19.
//

#ifndef LEARNFFMPEG_VIDEORENDER_H
#define LEARNFFMPEG_VIDEORENDER_H


#include "ImageDef.h"

class VideoRender {
public:
    virtual ~VideoRender(){}
    virtual void Init(int videoWidth, int videoHeight, int *dstSize) = 0;
    virtual void RenderVideoFrame(NativeImage *pImage) = 0;
    virtual void UnInit() = 0;
};


#endif //LEARNFFMPEG_VIDEORENDER_H
