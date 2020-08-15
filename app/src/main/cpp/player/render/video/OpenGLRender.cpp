//
// Created by 字节流动 on 2020/6/11.
//

#include "OpenGLRender.h"
#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>

OpenGLRender* OpenGLRender::s_Instance = nullptr;
std::mutex OpenGLRender::m_Mutex;

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
        "uniform sampler2D s_TextureMap;//采样器\n"
        "uniform vec2 u_TouchXY;//点击的位置（归一化）\n"
        "uniform vec2 u_TexSize;//纹理尺寸\n"
        "uniform float u_Time;//归一化的时间\n"
        "uniform float u_Boundary;//边界 0.1\n"
        "void main()\n"
        "{\n"
        "    float ratio = u_TexSize.y / u_TexSize.x;\n"
        "    vec2 texCoord = v_texCoord * vec2(1.0, ratio);//根据纹理尺寸，对采用坐标进行转换\n"
        "    vec2 touchXY = u_TouchXY * vec2(1.0, ratio);//根据纹理尺寸，对中心点坐标进行转换\n"
        "    float distance = distance(texCoord, touchXY);//采样点坐标与中心点的距离\n"
        "\n"
        "    if ((u_Time - u_Boundary) > 0.0\n"
        "    && (distance <= (u_Time + u_Boundary))\n"
        "    && (distance >= (u_Time - u_Boundary))) {\n"
        "        float diff = (distance - u_Time);\n"
        "        float moveDis = 20.0*diff*(diff - u_Boundary)*(diff + u_Boundary);//采样坐标移动距离\n"
        "        vec2 unitDirectionVec = normalize(texCoord - touchXY);//单位方向向量\n"
        "        texCoord = texCoord + (unitDirectionVec * moveDis);//采样坐标偏移（实现放大和缩小效果）\n"
        "    }\n"
        "\n"
        "    texCoord = texCoord / vec2(1.0, ratio);//转换回来\n"
        "    outColor = texture(s_TextureMap, texCoord);\n"
        "    if(texCoord.x > 0.5)\n"
        "        outColor = vec4(vec3(outColor.r*0.299 + outColor.g*0.587 + outColor.b*0.114), outColor.a);\n"
        "}";

GLfloat verticesCoords[] = {
        -1.0f,  1.0f, 0.0f,  // Position 0
        -1.0f, -1.0f, 0.0f,  // Position 1
        1.0f,  -1.0f, 0.0f,  // Position 2
        1.0f,   1.0f, 0.0f,  // Position 3
};

GLfloat textureCoords[] = {
        0.0f,  0.0f,        // TexCoord 0
        0.0f,  1.0f,        // TexCoord 1
        1.0f,  1.0f,        // TexCoord 2
        1.0f,  0.0f         // TexCoord 3
};

GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

OpenGLRender::OpenGLRender() {

}

OpenGLRender::~OpenGLRender() {
    NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

void OpenGLRender::Init(int videoWidth, int videoHeight, int *dstSize) {
    LOGCATE("OpenGLRender::InitRender video[w, h]=[%d, %d]", videoWidth, videoHeight);
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_RenderImage.format = IMAGE_FORMAT_RGBA;
    m_RenderImage.width = videoWidth;
    m_RenderImage.height = videoHeight;
    dstSize[0] = videoWidth;
    dstSize[1] = videoHeight;
    m_FrameIndex = 0;

}

void OpenGLRender::RenderVideoFrame(NativeImage *pImage) {
    LOGCATE("OpenGLRender::RenderVideoFrame pImage=%p", pImage);
    if(pImage == nullptr || pImage->ppPlane[0] == nullptr)
        return;
    std::unique_lock<std::mutex> lock(m_Mutex);
    if(m_RenderImage.ppPlane[0] == nullptr)
    {
        NativeImageUtil::AllocNativeImage(&m_RenderImage);
    }

    NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
}

void OpenGLRender::UnInit() {

}

void OpenGLRender::UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY)
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

void OpenGLRender::OnSurfaceCreated() {
    LOGCATE("OpenGLRender::OnSurfaceCreated");

    m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    if (!m_ProgramObj)
    {
        LOGCATE("OpenGLRender::OnSurfaceCreated create program fail");
        return;
    }

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

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

    UpdateMVPMatrix(0, 0, 1.0f, 1.0f);
    m_TouchXY = vec2(0.5f, 0.5f);
}

void OpenGLRender::OnSurfaceChanged(int w, int h) {
    LOGCATE("OpenGLRender::OnSurfaceChanged [w, h]=[%d, %d]", w, h);
    m_ScreenSize.x = w;
    m_ScreenSize.y = h;
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void OpenGLRender::OnDrawFrame() {
    glClear(GL_COLOR_BUFFER_BIT);
    if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE || m_RenderImage.ppPlane[0] == nullptr) return;
    LOGCATE("OpenGLRender::OnDrawFrame [w, h]=[%d, %d]", m_RenderImage.width, m_RenderImage.height);
    m_FrameIndex++;

    //upload RGBA image data
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);

    std::unique_lock<std::mutex> lock(m_Mutex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
    lock.unlock();

    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    // Use the program object
    glUseProgram (m_ProgramObj);

    glBindVertexArray(m_VaoId);

    GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", m_MVPMatrix);

    // Bind the RGBA map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    GLUtils::setFloat(m_ProgramObj, "s_TextureMap", 0);

    float time = static_cast<float>(fmod(m_FrameIndex, 60) / 50);
    GLUtils::setFloat(m_ProgramObj, "u_Time", time);

    GLUtils::setVec2(m_ProgramObj, "u_TouchXY", m_TouchXY);
    GLUtils::setVec2(m_ProgramObj, "u_TexSize", vec2(m_RenderImage.width, m_RenderImage.height));
    GLUtils::setFloat(m_ProgramObj, "u_Boundary", 0.1f);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

}

OpenGLRender *OpenGLRender::GetInstance() {
    if(s_Instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(s_Instance == nullptr)
        {
            s_Instance = new OpenGLRender();
        }

    }
    return s_Instance;
}

void OpenGLRender::ReleaseInstance() {
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


