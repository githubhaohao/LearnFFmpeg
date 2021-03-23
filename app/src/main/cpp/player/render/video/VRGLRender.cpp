/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include "VRGLRender.h"
#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>

VRGLRender* VRGLRender::s_Instance = nullptr;
std::mutex VRGLRender::m_Mutex;

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

static char fMeshShaderStr[] =
        "//dynimic mesh 动态网格\n"
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform sampler2D s_TextureMap;//采样器\n"
        "uniform float u_Offset;\n"
        "uniform vec2 u_TexSize;\n"
        "void main()\n"
        "{\n"
        "    vec2 imgTexCoord = v_texCoord * u_TexSize;\n"
        "    float sideLength = u_TexSize.y / 6.0;\n"
        "    float maxOffset = 0.15 * sideLength;\n"
        "    float x = mod(imgTexCoord.x, floor(sideLength));\n"
        "    float y = mod(imgTexCoord.y, floor(sideLength));\n"
        "\n"
        "    float offset = u_Offset * maxOffset;\n"
        "\n"
        "    if(offset <= x\n"
        "    && x <= sideLength - offset\n"
        "    && offset <= y\n"
        "    && y <= sideLength - offset)\n"
        "    {\n"
        "        outColor = texture(s_TextureMap, v_texCoord);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        outColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "    }\n"
        "}";

static char fGrayShaderStr[] =
        "//黑白滤镜\n"
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform sampler2D s_TextureMap;//采样器\n"
        "void main()\n"
        "{\n"
        "    outColor = texture(s_TextureMap, v_texCoord);\n"
        "    if(v_texCoord.x > 0.5)\n"
        "        outColor = vec4(vec3(outColor.r*0.299 + outColor.g*0.587 + outColor.b*0.114), outColor.a);\n"
        "}";

//GLfloat verticesCoords[] = {
//        -1.0f,  1.0f, 0.0f,  // Position 0
//        -1.0f, -1.0f, 0.0f,  // Position 1
//        1.0f,  -1.0f, 0.0f,  // Position 2
//        1.0f,   1.0f, 0.0f,  // Position 3
//};
//
//GLfloat textureCoords[] = {
//        0.0f,  0.0f,        // TexCoord 0
//        0.0f,  1.0f,        // TexCoord 1
//        1.0f,  1.0f,        // TexCoord 2
//        1.0f,  0.0f         // TexCoord 3
//};
//
//GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

VRGLRender::VRGLRender():VideoRender(VIDEO_RENDER_3D_VR) {

}

VRGLRender::~VRGLRender() {
    NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

void VRGLRender::Init(int videoWidth, int videoHeight, int *dstSize) {
    LOGCATE("VRGLRender::InitRender video[w, h]=[%d, %d]", videoWidth, videoHeight);
    dstSize[0] = videoWidth;
    dstSize[1] = videoHeight;
    m_FrameIndex = 0;
    //m_pSingleVideoRecorder = new SingleVideoRecorder("/sdcard/output.mp4", videoWidth, videoHeight, 400000, 25);
    //m_pSingleVideoRecorder->StartRecord();

}

void VRGLRender::RenderVideoFrame(NativeImage *pImage) {
    LOGCATE("VRGLRender::RenderVideoFrame pImage=%p, format=%d", pImage, pImage->format);
    if(pImage == nullptr || pImage->ppPlane[0] == nullptr)
        return;
    std::unique_lock<std::mutex> lock(m_Mutex);
    if(m_RenderImage.ppPlane[0] == nullptr)
    {
        m_RenderImage.format = pImage->format;
        m_RenderImage.width = pImage->width;
        m_RenderImage.height = pImage->height;
        NativeImageUtil::AllocNativeImage(&m_RenderImage);
    }

    NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
    //m_pSingleVideoRecorder->OnFrame2Encode(pImage);
}

void VRGLRender::UnInit() {
//    if(m_pSingleVideoRecorder != nullptr) {
//        m_pSingleVideoRecorder->StopRecord();
//        delete m_pSingleVideoRecorder;
//        m_pSingleVideoRecorder = nullptr;
//    }
}

void VRGLRender::UpdateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY)
{
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);
    // Projection matrix
    //glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    float ratio = m_ScreenSize.x / m_ScreenSize.y;
    glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 3), // Camera is at (0,0,1), in World Space
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

void VRGLRender::OnSurfaceCreated() {
    LOGCATE("VRGLRender::OnSurfaceCreated");

    m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    if (!m_ProgramObj)
    {
        LOGCATE("VRGLRender::OnSurfaceCreated create program fail");
        return;
    }
    GenerateMesh();

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
    glGenBuffers(2, m_VboIds);
    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * m_VertexCoords.size(), &m_VertexCoords[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * m_TextureCoords.size(), &m_TextureCoords[0], GL_STATIC_DRAW);

    // Generate VAO Id
    glGenVertexArrays(1, &m_VaoId);
    glBindVertexArray(m_VaoId);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindVertexArray(GL_NONE);

    m_TouchXY = vec2(0.5f, 0.5f);
}

void VRGLRender::OnSurfaceChanged(int w, int h) {
    LOGCATE("VRGLRender::OnSurfaceChanged [w, h]=[%d, %d]", w, h);
    m_ScreenSize.x = w;
    m_ScreenSize.y = h;
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    UpdateMVPMatrix(0, 0, 1.0f, 1.0f);
}

void VRGLRender::OnDrawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(m_ProgramObj == GL_NONE || m_RenderImage.ppPlane[0] == nullptr) return;
    LOGCATE("VRGLRender::OnDrawFrame [w, h]=[%d, %d]", m_RenderImage.width, m_RenderImage.height);
    m_FrameIndex++;
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // upload image data
    std::unique_lock<std::mutex> lock(m_Mutex);
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
    lock.unlock();


    // Use the program object
    glUseProgram (m_ProgramObj);

    glBindVertexArray(m_VaoId);

    GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", m_MVPMatrix);

    for (int i = 0; i < TEXTURE_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        char samplerName[64] = {0};
        sprintf(samplerName, "s_texture%d", i);
        GLUtils::setInt(m_ProgramObj, samplerName, i);
    }

    //float time = static_cast<float>(fmod(m_FrameIndex, 60) / 50);
    //GLUtils::setFloat(m_ProgramObj, "u_Time", time);

    float offset = (sin(m_FrameIndex * MATH_PI / 25) + 1.0f) / 2.0f;
    GLUtils::setFloat(m_ProgramObj, "u_Offset", offset);
    GLUtils::setVec2(m_ProgramObj, "u_TexSize", vec2(m_RenderImage.width, m_RenderImage.height));
    GLUtils::setInt(m_ProgramObj, "u_nImgType", m_RenderImage.format);

    glDrawArrays(GL_TRIANGLES, 0, m_VertexCoords.size());
    //glDrawArrays(GL_LINES, 0, m_VertexCoords.size());

}

VRGLRender *VRGLRender::GetInstance() {
    if(s_Instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(s_Instance == nullptr)
        {
            s_Instance = new VRGLRender();
        }

    }
    return s_Instance;
}

void VRGLRender::ReleaseInstance() {
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

void VRGLRender::GenerateMesh() {
    //构建顶点坐标
    for (float vAngle = 90; vAngle > -90; vAngle = vAngle - ANGLE_SPAN) {//垂直方向ANGLE_SPAN度一份
        for (float hAngle = 360; hAngle > 0; hAngle = hAngle - ANGLE_SPAN) {//水平方向ANGLE_SPAN度一份
            //纵向横向各到一个角度后计算对应的此点在球面上的坐标
            double xozLength = BALL_RADIUS * UNIT_SIZE * cos(RADIAN(vAngle));
            float x1 = (float) (xozLength * cos(RADIAN(hAngle)));
            float z1 = (float) (xozLength * sin(RADIAN(hAngle)));
            float y1 = (float) (BALL_RADIUS * UNIT_SIZE * sin(RADIAN(vAngle)));
            xozLength = BALL_RADIUS * UNIT_SIZE * cos(RADIAN(vAngle - ANGLE_SPAN));
            float x2 = (float) (xozLength * cos(RADIAN(hAngle)));
            float z2 = (float) (xozLength * sin(RADIAN(hAngle)));
            float y2 = (float) (BALL_RADIUS * UNIT_SIZE * sin(RADIAN(vAngle - ANGLE_SPAN)));
            xozLength = BALL_RADIUS * UNIT_SIZE * cos(RADIAN(vAngle - ANGLE_SPAN));
            float x3 = (float) (xozLength * cos(RADIAN(hAngle - ANGLE_SPAN)));
            float z3 = (float) (xozLength * sin(RADIAN(hAngle - ANGLE_SPAN)));
            float y3 = (float) (BALL_RADIUS * UNIT_SIZE * sin(RADIAN(vAngle - ANGLE_SPAN)));
            xozLength = BALL_RADIUS * UNIT_SIZE * cos(RADIAN(vAngle));
            float x4 = (float) (xozLength * cos(RADIAN(hAngle - ANGLE_SPAN)));
            float z4 = (float) (xozLength * sin(RADIAN(hAngle - ANGLE_SPAN)));
            float y4 = (float) (BALL_RADIUS * UNIT_SIZE * sin(RADIAN(vAngle)));

            vec3 v1(x1, y1, z1);
            vec3 v2(x2, y2, z2);
            vec3 v3(x3, y3, z3);
            vec3 v4(x4, y4, z4);

            //构建第一个三角形
            m_VertexCoords.push_back(v1);
            m_VertexCoords.push_back(v2);
            m_VertexCoords.push_back(v4);
            //构建第二个三角形
            m_VertexCoords.push_back(v4);
            m_VertexCoords.push_back(v2);
            m_VertexCoords.push_back(v3);
        }
    }

    //构建纹理坐标，球面展开后的矩形
    int width = 360 / ANGLE_SPAN;//列数
    int height = 180 / ANGLE_SPAN;//行数
    float dw = 1.0f / width;
    float dh = 1.0f / height;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            //每一个小矩形，由两个三角形构成，共六个点
            float s = j * dw;
            float t = i * dh;
            vec2 v1(s, t);
            vec2 v2(s, t + dh);
            vec2 v3(s + dw, t + dh);
            vec2 v4(s + dw, t);

            //构建第一个三角形
            m_TextureCoords.push_back(v1);
            m_TextureCoords.push_back(v2);
            m_TextureCoords.push_back(v4);
            //构建第二个三角形
            m_TextureCoords.push_back(v4);
            m_TextureCoords.push_back(v2);
            m_TextureCoords.push_back(v3);
        }
    }
}


