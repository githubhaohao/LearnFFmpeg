/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <LogUtil.h>
#include <GLUtils.h>
#include "AudioGLRender.h"
#include <gtc/matrix_transform.hpp>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <render/video/VideoGLRender.h>


AudioGLRender* AudioGLRender::m_pInstance = nullptr;
std::mutex AudioGLRender::m_Mutex;

AudioGLRender *AudioGLRender::GetInstance() {
    if(m_pInstance == nullptr) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        if(m_pInstance == nullptr) {
            m_pInstance = new AudioGLRender();
        }

    }
    return m_pInstance;
}

void AudioGLRender::ReleaseInstance() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if(m_pInstance != nullptr) {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

}

void AudioGLRender::OnSurfaceCreated() {
    ByteFlowPrintE("AudioGLRender::OnSurfaceCreated");
    if (m_ProgramObj)
        return;
    char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;\n"
            "layout(location = 1) in vec2 a_texCoord;\n"
            "uniform mat4 u_MVPMatrix;\n"
            "out vec2 v_texCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * a_position;\n"
            "    v_texCoord = a_texCoord;\n"
            "    gl_PointSize = 4.0f;\n"
            "}";

    char fShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "in vec2 v_texCoord;                                 \n"
            "layout(location = 0) out vec4 outColor;             \n"
            "uniform float drawType;                             \n"
            "void main()                                         \n"
            "{                                                   \n"
            "  if(drawType == 1.0)                               \n"
            "  {                                                 \n"
            "      outColor = vec4(1.5 - v_texCoord.y, 0.3, 0.3, 1.0); \n"
            "  }                                                 \n"
            "  else if(drawType == 2.0)                          \n"
            "  {                                                 \n"
            "      outColor = vec4(1.0, 1.0, 1.0, 1.0);          \n"
            "  }                                                 \n"
            "  else if(drawType == 3.0)                          \n"
            "  {                                                 \n"
            "      outColor = vec4(0.3, 0.3, 0.3, 1.0);          \n"
            "  }                                                 \n"
            "}                                                   \n";

    m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    if (m_ProgramObj == GL_NONE) {
        LOGCATE("VisualizeAudioSample::Init create program fail");
    }


    //设置 MVP Matrix
    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f, ratio, 0.1f, 100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(1.0f, 1.0f, 1.0f));
    Model = glm::rotate(Model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    m_MVPMatrix = Projection * View * Model;

}

void AudioGLRender::OnSurfaceChanged(int w, int h) {
    ByteFlowPrintE("AudioGLRender::OnSurfaceChanged [w, h] = [%d, %d]", w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0);
    glViewport(0, 0, w, h);

}

void AudioGLRender::OnDrawFrame() {
    ByteFlowPrintD("AudioGLRender::OnDrawFrame");
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (m_ProgramObj == GL_NONE || m_pAudioBuffer == nullptr) return;
    UpdateMesh();
    lock.unlock();

    // Generate VBO Ids and load the VBOs with data
    if(m_VboIds[0] == 0)
    {
        glGenBuffers(2, m_VboIds);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m_RenderDataSize * 6 * 3, m_pVerticesCoords, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m_RenderDataSize * 6 * 2, m_pTextureCoords, GL_DYNAMIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * m_RenderDataSize * 6 * 3, m_pVerticesCoords);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * m_RenderDataSize * 6 * 2, m_pTextureCoords);
    }

    if(m_VaoId == GL_NONE)
    {
        glGenVertexArrays(1, &m_VaoId);
        glBindVertexArray(m_VaoId);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        glBindVertexArray(GL_NONE);
    }

    // Use the program object
    glUseProgram(m_ProgramObj);
    glBindVertexArray(m_VaoId);
    GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", m_MVPMatrix);
    GLUtils::setFloat(m_ProgramObj, "drawType", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, m_RenderDataSize * 6);
    GLUtils::setFloat(m_ProgramObj, "drawType", 2.0f);
    glDrawArrays(GL_LINES, 0, m_RenderDataSize * 6);

}

void AudioGLRender::UpdateAudioFrame(AudioFrame *audioFrame) {
    if(audioFrame != nullptr) {
        ByteFlowPrintD("AudioGLRender::UpdateAudioFrame audioFrame->dataSize=%d", audioFrame->dataSize);
        std::unique_lock<std::mutex> lock(m_Mutex);
        if(m_pAudioBuffer != nullptr && m_pAudioBuffer->dataSize != audioFrame->dataSize) {
            delete m_pAudioBuffer;
            m_pAudioBuffer = nullptr;

            delete [] m_pTextureCoords;
            m_pTextureCoords = nullptr;

            delete [] m_pVerticesCoords;
            m_pVerticesCoords = nullptr;
        }

        if(m_pAudioBuffer == nullptr) {
            m_pAudioBuffer = new AudioFrame(audioFrame->data, audioFrame->dataSize);
            m_RenderDataSize = m_pAudioBuffer->dataSize / RESAMPLE_LEVEL;

            m_pVerticesCoords = new vec3[m_RenderDataSize * 6]; //(x,y,z) * 6 points
            m_pTextureCoords = new vec2[m_RenderDataSize * 6]; //(x,y) * 6 points
        } else {
            memcpy(m_pAudioBuffer->data, audioFrame->data, audioFrame->dataSize);
        }
        lock.unlock();
    }
}

void AudioGLRender::UpdateMesh() {
    float dy = 0.25f / MAX_AUDIO_LEVEL;
    float dx = 1.0f / m_RenderDataSize;
    for (int i = 0; i < m_RenderDataSize; ++i) {
        int index = i * RESAMPLE_LEVEL;
        short *pValue = (short *)(m_pAudioBuffer->data + index);
        float y = *pValue * dy;
        y = y < 0 ? y : -y;
        vec2 p1(i * dx, 0 + 1.0f);
        vec2 p2(i * dx, y + 1.0f);
        vec2 p3((i + 1) * dx, y + 1.0f);
        vec2 p4((i + 1) * dx, 0 + 1.0f);

        m_pTextureCoords[i * 6 + 0] = p1;
        m_pTextureCoords[i * 6 + 1] = p2;
        m_pTextureCoords[i * 6 + 2] = p4;
        m_pTextureCoords[i * 6 + 3] = p4;
        m_pTextureCoords[i * 6 + 4] = p2;
        m_pTextureCoords[i * 6 + 5] = p3;

        m_pVerticesCoords[i * 6 + 0] = GLUtils::texCoordToVertexCoord(p1);
        m_pVerticesCoords[i * 6 + 1] = GLUtils::texCoordToVertexCoord(p2);
        m_pVerticesCoords[i * 6 + 2] = GLUtils::texCoordToVertexCoord(p4);
        m_pVerticesCoords[i * 6 + 3] = GLUtils::texCoordToVertexCoord(p4);
        m_pVerticesCoords[i * 6 + 4] = GLUtils::texCoordToVertexCoord(p2);
        m_pVerticesCoords[i * 6 + 5] = GLUtils::texCoordToVertexCoord(p3);
    }
}

void AudioGLRender::Init() {
    m_VaoId = GL_NONE;

    m_pTextureCoords = nullptr;
    m_pVerticesCoords = nullptr;

    memset(m_VboIds, 0, sizeof(GLuint) * 2);
    m_pAudioBuffer = nullptr;

}

void AudioGLRender::UnInit() {
    if (m_pAudioBuffer != nullptr) {
        delete m_pAudioBuffer;
        m_pAudioBuffer = nullptr;
    }

    if (m_pTextureCoords != nullptr) {
        delete [] m_pTextureCoords;
        m_pTextureCoords = nullptr;
    }

    if (m_pVerticesCoords != nullptr) {
        delete [] m_pVerticesCoords;
        m_pVerticesCoords = nullptr;
    }
}

