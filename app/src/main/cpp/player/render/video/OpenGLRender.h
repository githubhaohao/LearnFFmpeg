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

using namespace glm;

#define MATH_PI 3.1415926535897932384626433832802

class OpenGLRender: public VideoRender{
public:
    virtual void Init(int videoWidth, int videoHeight, int *dstSize);
    virtual void RenderVideoFrame(NativeImage *pImage);
    virtual void UnInit();

    void OnSurfaceCreated();
    void OnSurfaceChanged(int w, int h);
    void OnDrawFrame();

    static OpenGLRender *GetInstance();
    static void ReleaseInstance();

    void UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY);

    void SetTouchLoc(float touchX, float touchY) {
        m_TouchXY.x = touchX / m_ScreenSize.x;
        m_TouchXY.y = touchY / m_ScreenSize.y;
    }

private:
    OpenGLRender();
    virtual ~OpenGLRender();

    static std::mutex m_Mutex;
    static OpenGLRender* s_Instance;
    GLuint m_ProgramObj = GL_NONE;
    GLuint m_TextureId;
    GLuint m_VaoId;
    GLuint m_VboIds[3];
    NativeImage m_RenderImage;
    glm::mat4 m_MVPMatrix;

    int m_FrameIndex;
    vec2 m_TouchXY;
    vec2 m_ScreenSize;
};


#endif //LEARNFFMPEG_MASTER_GLRENDER_H
