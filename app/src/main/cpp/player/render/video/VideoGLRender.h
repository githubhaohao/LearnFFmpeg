//
// Created by 字节流动 on 2020/6/11.
//

#ifndef LEARNFFMPEG_MASTER_GLRENDER_H
#define LEARNFFMPEG_MASTER_GLRENDER_H
#include <thread>
#include <ImageDef.h>
#include "VideoRender.h"
#include <GLES3/gl3.h>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <vec2.hpp>
#include <render/BaseGLRender.h>

using namespace glm;

#define MATH_PI 3.1415926535897932384626433832802
#define TEXTURE_NUM 3

class VideoGLRender: public VideoRender, public BaseGLRender{
public:
    virtual void Init(int videoWidth, int videoHeight, int *dstSize);
    virtual void RenderVideoFrame(NativeImage *pImage);
    virtual void UnInit();

    virtual void OnSurfaceCreated();
    virtual void OnSurfaceChanged(int w, int h);
    virtual void OnDrawFrame();

    static VideoGLRender *GetInstance();
    static void ReleaseInstance();

    virtual void UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY);
    virtual void SetTouchLoc(float touchX, float touchY) {
        m_TouchXY.x = touchX / m_ScreenSize.x;
        m_TouchXY.y = touchY / m_ScreenSize.y;
    }

private:
    VideoGLRender();
    virtual ~VideoGLRender();

    static std::mutex m_Mutex;
    static VideoGLRender* s_Instance;
    GLuint m_ProgramObj = GL_NONE;
    GLuint m_TextureIds[TEXTURE_NUM];
    GLuint m_VaoId;
    GLuint m_VboIds[3];
    NativeImage m_RenderImage;
    glm::mat4 m_MVPMatrix;

    int m_FrameIndex;
    vec2 m_TouchXY;
    vec2 m_ScreenSize;
};


#endif //LEARNFFMPEG_MASTER_GLRENDER_H
