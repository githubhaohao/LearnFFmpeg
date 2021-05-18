/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_ASANTESTCASE_H
#define LEARNFFMPEG_ASANTESTCASE_H
#include "LogUtil.h"

//#define LOGCATE LOGCATE

class ASanTestCase {
    //heap-buffer-overflow
    static void HeapBufferOverflow() {
        int *arr = new int[1024];
        arr[0] = 11;
        arr[1024] = 12;
        LOGCATE("HeapBufferOverflow arr[0]=%d, arr[1024]",arr[0], arr[1024]);
    }

    //stack-buffer-overflow
    static void StackBufferOverflow() {
        int arr[1024];
        arr[0] = 11;
        arr[1024] = 12;
        LOGCATE("StackBufferOverflow arr[0]=%d, arr[1024]",arr[0], arr[1024]);
    }

    //heap-use-after-free
    static void UseAfterFree() {
        int *arr = new int[1024];
        arr[0] = 11;
        delete [] arr;
        LOGCATE("UseAfterFree arr[0]=%d, arr[1024]",arr[0], arr[1024]);
    }

    //double-free
    static void DoubleFree() {
        int *arr = new int[1024];
        arr[0] = 11;
        delete [] arr;
        delete [] arr;
        LOGCATE("UseAfterFree arr[0]=%d",arr[0]);
    }

    //stack-use-after-scope
    static int *p;
    static void UseAfterScope()
    {
        {
            int a = 0;
            p = &a;
        }
        *p = 1111;
        LOGCATE("UseAfterScope *p=%d",*p);
    }

public:
    static void MainTest() {
        HeapBufferOverflow();
        StackBufferOverflow();
        UseAfterFree();
        UseAfterScope();
        DoubleFree();
    }
};
int *ASanTestCase::p = nullptr;
#endif //LEARNFFMPEG_ASANTESTCASE_H
