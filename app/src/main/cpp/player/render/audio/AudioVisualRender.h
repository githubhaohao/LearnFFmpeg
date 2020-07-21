//
// Created by chh7563 on 2020/7/14.
//

#ifndef LEARNFFMPEG_AUDIOVISUALRENDER_H
#define LEARNFFMPEG_AUDIOVISUALRENDER_H

#include "thread"
#include "AudioRender.h"
#include <GLES3/gl3.h>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>

using namespace glm;

#define MAX_AUDIO_LEVEL 5000
#define RESAMPLE_LEVEL  40

class AudioVisualRender {
public:
    static AudioVisualRender* GetInstance();
    static void ReleaseInstance();

    void OnAudioVisualSurfaceCreated();
    void OnAudioVisualSurfaceChanged(int w, int h);
    void OnAudioVisualDrawFrame();

    void UpdateAudioFrame(AudioFrame *audioFrame);

private:
    void Init();
    void UnInit();
    AudioVisualRender(){
        Init();
    }
    ~AudioVisualRender(){
        UnInit();
    }

    void UpdateMesh();

    static AudioVisualRender *m_pInstance;
    static std::mutex m_Mutex;
    AudioFrame *m_pAudioBuffer = nullptr;

    GLuint m_ProgramObj;
    GLuint m_VaoId;
    GLuint m_VboIds[2];
    glm::mat4 m_MVPMatrix;

    vec3 *m_pVerticesCoords = nullptr;
    vec2 *m_pTextureCoords = nullptr;

    int m_RenderDataSize;

};


#endif //LEARNFFMPEG_AUDIOVISUALRENDER_H
