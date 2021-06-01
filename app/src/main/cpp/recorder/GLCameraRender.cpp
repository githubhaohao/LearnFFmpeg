/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "GLCameraRender.h"
#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>

GLCameraRender* GLCameraRender::s_Instance = nullptr;
std::mutex GLCameraRender::m_Mutex;

static char vShaderStr[] =
        "#version 300 es\n"
        "layout(location = 0) in vec4 a_position;\n"
        "layout(location = 1) in vec2 a_texCoord;\n"
        "uniform mat4 u_MVPMatrix;\n"
        "out vec2 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = u_MVPMatrix * a_position;\n"
        "    v_texCoord = a_texCoord;\n"
        "}";

static char fShaderStr[] =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform sampler2D s_texture0;\n"
        "uniform sampler2D s_texture1;\n"
        "uniform sampler2D s_texture2;\n"
        "uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    if(u_nImgType == 1) //RGBA\n"
        "    {\n"
        "        outColor = texture(s_texture0, v_texCoord);\n"
        "    }\n"
        "    else if(u_nImgType == 2) //NV21\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "\n"
        "    }\n"
        "    else if(u_nImgType == 3) //NV12\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else if(u_nImgType == 4) //I420\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture2, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "                              0.0, \t-0.344, \t1.770,\n"
        "                              1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        outColor = vec4(1.0);\n"
        "    }\n"
        "}";

static GLfloat verticesCoords[] = {
        -1.0f,  1.0f, 0.0f,  // Position 0
        -1.0f, -1.0f, 0.0f,  // Position 1
        1.0f,  -1.0f, 0.0f,  // Position 2
        1.0f,   1.0f, 0.0f,  // Position 3
};

//static GLfloat textureCoords[] = {
//        0.0f,  1.0f,        // TexCoord 0
//        0.0f,  0.0f,        // TexCoord 1
//        1.0f,  0.0f,        // TexCoord 2
//        1.0f,  1.0f         // TexCoord 3
//};
static GLfloat textureCoords[] = {
        0.0f,  0.0f,        // TexCoord 0
        0.0f,  1.0f,        // TexCoord 1
        1.0f,  1.0f,        // TexCoord 2
        1.0f,  0.0f         // TexCoord 3
};

static GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

GLCameraRender::GLCameraRender():VideoRender(VIDEO_RENDER_OPENGL) {

}

GLCameraRender::~GLCameraRender() {
    NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

void GLCameraRender::Init(int videoWidth, int videoHeight, int *dstSize) {
    LOGCATE("GLCameraRender::InitRender video[w, h]=[%d, %d]", videoWidth, videoHeight);
    if(dstSize != nullptr) {
        dstSize[0] = videoWidth;
        dstSize[1] = videoHeight;
    }
    m_FrameIndex = 0;
    UpdateMVPMatrix(0, 0, 1.0f, 1.0f);
}

void GLCameraRender::RenderVideoFrame(NativeImage *pImage) {
    LOGCATE("GLCameraRender::RenderVideoFrame pImage=%p", pImage);
    if(pImage == nullptr || pImage->ppPlane[0] == nullptr)
        return;
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (pImage->width != m_RenderImage.width || pImage->height != m_RenderImage.height) {
        if (m_RenderImage.ppPlane[0] != nullptr) {
            NativeImageUtil::FreeNativeImage(&m_RenderImage);
        }
        memset(&m_RenderImage, 0, sizeof(NativeImage));
        m_RenderImage.format = pImage->format;
        m_RenderImage.width = pImage->width;
        m_RenderImage.height = pImage->height;
        NativeImageUtil::AllocNativeImage(&m_RenderImage);
    }

    NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
    //NativeImageUtil::DumpNativeImage(&m_RenderImage, "/sdcard", "camera");
}

void GLCameraRender::UnInit() {
    NativeImageUtil::FreeNativeImage(&m_ExtImage);

    if(m_pFragShaderBuffer != nullptr) {
        free(m_pFragShaderBuffer);
        m_pFragShaderBuffer = nullptr;
    }

}

void GLCameraRender::UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY)
{
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);
    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(scaleX, scaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    m_MVPMatrix = Projection * View * Model;

}

void GLCameraRender::UpdateMVPMatrix(TransformMatrix *pTransformMatrix) {
    //BaseGLRender::UpdateMVPMatrix(pTransformMatrix);
    m_transformMatrix = *pTransformMatrix;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * pTransformMatrix->angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * pTransformMatrix->angleY);

    float fFactorX = 1.0f;
    float fFactorY = 1.0f;

    if (pTransformMatrix->mirror == 1) {
        fFactorX = -1.0f;
    } else if (pTransformMatrix->mirror == 2) {
        fFactorY = -1.0f;
    }

    float fRotate = MATH_PI * pTransformMatrix->degree * 1.0f / 180;
    if (pTransformMatrix->mirror == 0) {
        if (pTransformMatrix->degree == 270) {
            fRotate = MATH_PI * 0.5;
        } else if (pTransformMatrix->degree == 180) {
            fRotate = MATH_PI;
        } else if (pTransformMatrix->degree == 90) {
            fRotate = MATH_PI * 1.5;
        }
    } else if (pTransformMatrix->mirror == 1) {
        if (pTransformMatrix->degree == 90) {
            fRotate = MATH_PI * 0.5;
        } else if (pTransformMatrix->degree == 180) {
            fRotate = MATH_PI;
        } else if (pTransformMatrix->degree == 270) {
            fRotate = MATH_PI * 1.5;
        }
    }

    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 1), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0) // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(fFactorX * pTransformMatrix->scaleX,
                                        fFactorY * pTransformMatrix->scaleY, 1.0f));
    Model = glm::rotate(Model, fRotate, glm::vec3(0.0f, 0.0f, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model,
                           glm::vec3(pTransformMatrix->translateX, pTransformMatrix->translateY, 0.0f));

    LOGCATE("GLCameraRender::UpdateMVPMatrix rotate %d,%.2f,%0.5f,%0.5f,%0.5f,%0.5f,", pTransformMatrix->degree, fRotate,
            pTransformMatrix->translateX, pTransformMatrix->translateY,
            fFactorX * pTransformMatrix->scaleX, fFactorY * pTransformMatrix->scaleY);

    m_MVPMatrix = Projection * View * Model;
}

void GLCameraRender::OnSurfaceCreated() {
    LOGCATE("GLCameraRender::OnSurfaceCreated");

    m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    m_FboProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    if (!m_ProgramObj || !m_FboProgramObj)
    {
        LOGCATE("GLCameraRender::OnSurfaceCreated create program fail");
        return;
    }

    glGenTextures(TEXTURE_NUM, m_TextureIds);
    for (int i = 0; i < TEXTURE_NUM ; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    // Generate VBO Ids and load the VBOs with data
    glGenBuffers(3, m_VboIds);
    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Generate VAO Id
    glGenVertexArrays(1, &m_VaoId);
    glBindVertexArray(m_VaoId);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

    glBindVertexArray(GL_NONE);

    m_TouchXY = vec2(0.5f, 0.5f);
}

void GLCameraRender::OnSurfaceChanged(int w, int h) {
    LOGCATE("GLCameraRender::OnSurfaceChanged [w, h]=[%d, %d]", w, h);
    m_ScreenSize.x = w;
    m_ScreenSize.y = h;
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void GLCameraRender::OnDrawFrame() {
    if(m_IsShaderChanged) {
        unique_lock<mutex> lock(m_ShaderMutex);
        GLUtils::DeleteProgram(m_FboProgramObj);
        m_FboProgramObj = GLUtils::CreateProgram(vShaderStr, m_pFragShaderBuffer);
        m_IsShaderChanged = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    if(m_ProgramObj == GL_NONE || m_RenderImage.ppPlane[0] == nullptr) return;
    if(m_SrcFboId == GL_NONE && CreateFrameBufferObj()) {
        LOGCATE("GLCameraRender::OnDrawFrame CreateFrameBufferObj fail");
        return;
    }
    LOGCATE("GLCameraRender::OnDrawFrame [w, h]=[%d, %d], format=%d", m_RenderImage.width, m_RenderImage.height, m_RenderImage.format);
    m_FrameIndex++;

    UpdateExtTexture();

    std::unique_lock<std::mutex> lock(m_Mutex);
    // 渲染到 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_SrcFboId);
    glViewport(0, 0, m_RenderImage.height, m_RenderImage.width); //相机的宽和高反了
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_FboProgramObj);
    // upload image data

    glBindTexture(GL_TEXTURE_2D, m_SrcFboTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.height, m_RenderImage.width, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, m_DstFboTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.height, m_RenderImage.width, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    switch (m_RenderImage.format)
    {
        case IMAGE_FORMAT_RGBA:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_NV21:
        case IMAGE_FORMAT_NV12:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width,
                         m_RenderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update UV plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_I420:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width,
                         m_RenderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update U plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update V plane data
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_TextureIds[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width >> 1,
                         m_RenderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         m_RenderImage.ppPlane[2]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        default:
            break;
    }

    glBindVertexArray(m_VaoId);
    UpdateMVPMatrix(&m_transformMatrix);
    GLUtils::setMat4(m_FboProgramObj, "u_MVPMatrix", m_MVPMatrix);
    for (int i = 0; i < TEXTURE_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        char samplerName[64] = {0};
        sprintf(samplerName, "s_texture%d", i);
        GLUtils::setInt(m_FboProgramObj, samplerName, i);
    }
    float offset = (sin(m_FrameIndex * MATH_PI / 40) + 1.0f) / 2.0f;
    GLUtils::setFloat(m_FboProgramObj, "u_Offset", offset);
    GLUtils::setVec2(m_FboProgramObj, "u_TexSize", vec2(m_RenderImage.width, m_RenderImage.height));
    GLUtils::setInt(m_FboProgramObj, "u_nImgType", m_RenderImage.format);

    switch (m_ShaderIndex) {
        case SHADER_INDEX_ORIGIN:
            break;
        case SHADER_INDEX_DMESH:
            break;
        case SHADER_INDEX_GHOST:
            offset = m_FrameIndex % 60 / 60.0f - 0.2f;
            if(offset < 0) offset = 0;
            GLUtils::setFloat(m_FboProgramObj, "u_Offset", offset);
            break;
        case SHADER_INDEX_CIRCLE:
            break;
        case SHADER_INDEX_ASCII:
            glActiveTexture(GL_TEXTURE0 + TEXTURE_NUM);
            glBindTexture(GL_TEXTURE_2D, m_ExtTextureId);
            GLUtils::setInt(m_FboProgramObj, "s_textureMapping", TEXTURE_NUM);
            GLUtils::setVec2(m_FboProgramObj, "asciiTexSize", vec2(m_ExtImage.width, m_ExtImage.height));
            break;
        case SHADER_INDEX_LUT_A:
        case SHADER_INDEX_LUT_B:
        case SHADER_INDEX_LUT_C:
            glActiveTexture(GL_TEXTURE0 + TEXTURE_NUM);
            glBindTexture(GL_TEXTURE_2D, m_ExtTextureId);
            GLUtils::setInt(m_FboProgramObj, "s_LutTexture", TEXTURE_NUM);
            break;
        case SHADER_INDEX_NE:
            offset = (sin(m_FrameIndex * MATH_PI / 60) + 1.0f) / 2.0f;
            GLUtils::setFloat(m_FboProgramObj, "u_Offset", offset);
            break;
        default:
            break;
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

    //再绘制一次，把方向倒过来
    glBindFramebuffer(GL_FRAMEBUFFER, m_DstFboId);
    glViewport(0, 0, m_RenderImage.height, m_RenderImage.width); //相机的宽和高反了,
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram (m_ProgramObj);
    glBindVertexArray(m_VaoId);

    UpdateMVPMatrix(0, 0, 1.0, 1.0);
    GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", m_MVPMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_SrcFboTextureId);
    GLUtils::setInt(m_ProgramObj, "s_texture0", 0);
    GLUtils::setInt(m_ProgramObj, "u_nImgType", IMAGE_FORMAT_RGBA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

    GetRenderFrameFromFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    lock.unlock();

    // 渲染到屏幕
    glViewport(0, 0, m_ScreenSize.x, m_ScreenSize.y);
    glClear(GL_COLOR_BUFFER_BIT);

    UpdateMVPMatrix(0, 0, 1.0, 1.0);
    GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", m_MVPMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_DstFboTextureId);
    GLUtils::setInt(m_ProgramObj, "s_texture0", 0);

    GLUtils::setInt(m_ProgramObj, "u_nImgType", IMAGE_FORMAT_RGBA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
}

GLCameraRender *GLCameraRender::GetInstance() {
    if(s_Instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(s_Instance == nullptr)
        {
            s_Instance = new GLCameraRender();
        }

    }
    return s_Instance;
}

void GLCameraRender::ReleaseInstance() {
    if(s_Instance != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(s_Instance != nullptr)
        {
            delete s_Instance;
            s_Instance = nullptr;
        }

    }
}

bool GLCameraRender::CreateFrameBufferObj() {
    // 创建并初始化 FBO 纹理
    if(m_SrcFboTextureId == GL_NONE) {
        glGenTextures(1, &m_SrcFboTextureId);
        glBindTexture(GL_TEXTURE_2D, m_SrcFboTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    if(m_DstFboTextureId == GL_NONE) {
        glGenTextures(1, &m_DstFboTextureId);
        glBindTexture(GL_TEXTURE_2D, m_DstFboTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    // 创建并初始化 FBO
    if(m_SrcFboId == GL_NONE) {
        glGenFramebuffers(1, &m_SrcFboId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_SrcFboId);
        glBindTexture(GL_TEXTURE_2D, m_SrcFboTextureId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SrcFboTextureId, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.height, m_RenderImage.width, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
            LOGCATE("GLCameraRender::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            if(m_SrcFboTextureId != GL_NONE) {
                glDeleteTextures(1, &m_SrcFboTextureId);
                m_SrcFboTextureId = GL_NONE;
            }

            if(m_SrcFboId != GL_NONE) {
                glDeleteFramebuffers(1, &m_SrcFboId);
                m_SrcFboId = GL_NONE;
            }
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
            return false;
        }
    }

    if(m_DstFboId == GL_NONE) {
        glGenFramebuffers(1, &m_DstFboId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_DstFboId);
        glBindTexture(GL_TEXTURE_2D, m_DstFboTextureId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_DstFboTextureId, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.height, m_RenderImage.width, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
            LOGCATE("GLCameraRender::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            if(m_DstFboTextureId != GL_NONE) {
                glDeleteTextures(1, &m_DstFboTextureId);
                m_DstFboTextureId = GL_NONE;
            }

            if(m_DstFboId != GL_NONE) {
                glDeleteFramebuffers(1, &m_DstFboId);
                m_DstFboId = GL_NONE;
            }
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
            return false;
        }
    }

    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    return true;
}

void GLCameraRender::GetRenderFrameFromFBO() {
    LOGCATE("GLCameraRender::GetRenderFrameFromFBO m_RenderFrameCallback=%p", m_RenderFrameCallback);
    if(m_RenderFrameCallback != nullptr) {
        uint8_t *pBuffer = new uint8_t[m_RenderImage.width * m_RenderImage.height * 4];
        NativeImage nativeImage = m_RenderImage;
        nativeImage.format = IMAGE_FORMAT_RGBA;
        nativeImage.width = m_RenderImage.height;
        nativeImage.height = m_RenderImage.width;
        nativeImage.pLineSize[0] = nativeImage.width * 4;
        nativeImage.ppPlane[0] = pBuffer;
        glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
        m_RenderFrameCallback(m_CallbackContext, &nativeImage);
        delete []pBuffer;
    }
}

void GLCameraRender::SetFragShaderStr(int index, char *pShaderStr, int strSize) {
    LOGCATE("GLByteFlowRender::LoadFragShaderScript pShaderStr = %p, shaderIndex=%d", pShaderStr,
            index);
    if(m_ShaderIndex != index) {
        unique_lock<mutex> lock(m_ShaderMutex);
        if(m_pFragShaderBuffer != nullptr) {
            free(m_pFragShaderBuffer);
            m_pFragShaderBuffer = nullptr;
        }
        m_ShaderIndex = index;
        m_pFragShaderBuffer = static_cast<char *>(malloc(strSize));
        memcpy(m_pFragShaderBuffer, pShaderStr, strSize);
        m_IsShaderChanged = true;
    }

}

void GLCameraRender::SetLUTImage(int index, NativeImage *pLUTImg) {
    LOGCATE("GLCameraRender::SetLUTImage pImage = %p, index=%d", pLUTImg->ppPlane[0],
            index);
    unique_lock<mutex> lock(m_Mutex);
    NativeImageUtil::FreeNativeImage(&m_ExtImage);
    m_ExtImage.width = pLUTImg->width;
    m_ExtImage.height = pLUTImg->height;
    m_ExtImage.format = pLUTImg->format;
    NativeImageUtil::AllocNativeImage(&m_ExtImage);
    NativeImageUtil::CopyNativeImage(pLUTImg, &m_ExtImage);
    m_ExtImageChanged = true;
}

void GLCameraRender::UpdateExtTexture() {
    LOGCATE("GLCameraRender::UpdateExtTexture");
    if(m_ExtImageChanged && m_ExtImage.ppPlane[0] != nullptr) {
        if(m_ExtTextureId != GL_NONE) {
            glDeleteTextures(1, &m_ExtTextureId);
        }
        glGenTextures(1, &m_ExtTextureId);
        glBindTexture(GL_TEXTURE_2D, m_ExtTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_ExtImage.width, m_ExtImage.height, 0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE, m_ExtImage.ppPlane[0]);
        m_ExtImageChanged = false;
    }
}



