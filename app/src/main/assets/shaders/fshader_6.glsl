#version 300 es
precision highp float;
in vec2 v_texCoord;
layout(location = 0) out vec4 outColor;
uniform sampler2D s_texture0;
uniform sampler2D s_texture1;
uniform sampler2D s_texture2;
uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420
uniform float u_Offset;
uniform vec2 u_TexSize;

/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * 磨砂
 * */

vec4 sampleImage(vec2 texCoord) {
    vec4 outColor;
    if(u_nImgType == 1) //RGBA
    {
        outColor = texture(s_texture0, texCoord);
    }
    else if(u_nImgType == 2) //NV21
    {
        vec3 yuv;
        yuv.x = texture(s_texture0, texCoord).r;
        yuv.y = texture(s_texture1, texCoord).a - 0.5;
        yuv.z = texture(s_texture1, texCoord).r - 0.5;
        highp vec3 rgb = mat3(1.0,       1.0,     1.0,
        0.0,  -0.344,  1.770,
        1.403,  -0.714,     0.0) * yuv;
        outColor = vec4(rgb, 1.0);

    }
    else if(u_nImgType == 3) //NV12
    {
        vec3 yuv;
        yuv.x = texture(s_texture0, texCoord).r;
        yuv.y = texture(s_texture1, texCoord).r - 0.5;
        yuv.z = texture(s_texture1, texCoord).a - 0.5;
        highp vec3 rgb = mat3(1.0,       1.0,     1.0,
        0.0,  -0.344,  1.770,
        1.403,  -0.714,     0.0) * yuv;
        outColor = vec4(rgb, 1.0);
    }
    else if(u_nImgType == 4) //I420
    {
        vec3 yuv;
        yuv.x = texture(s_texture0, texCoord).r;
        yuv.y = texture(s_texture1, texCoord).r - 0.5;
        yuv.z = texture(s_texture2, texCoord).r - 0.5;
        highp vec3 rgb = mat3(1.0,       1.0,     1.0,
        0.0,  -0.344,  1.770,
        1.403,  -0.714,     0.0) * yuv;
        outColor = vec4(rgb, 1.0);
    }
    else
    {
        outColor = vec4(1.0);
    }
    return outColor;
}

const float arg = 0.5;
const vec2 samplerSteps = vec2(1.0, 1.0);
const float blurSamplerScale = 4.0;
const float factor = 0.1;
const int samplerRadius = 2;

float random(vec2 seed)
{
    return fract(sin(dot(seed ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec4 centralColor = sampleImage(v_texCoord);
    float lum = dot(centralColor.rgb, vec3(0.299, 0.587, 0.114));
    float factor = (1.0 + arg) / (arg + lum) * factor;

    float gaussianWeightTotal = 1.0;
    vec4 sum = centralColor * gaussianWeightTotal;
    vec2 stepScale = blurSamplerScale * samplerSteps / float(samplerRadius);
    float offset = random(v_texCoord) - 0.5;

    for(int i = 1; i <= samplerRadius; ++i)
    {
        vec2 dis = (float(i) + offset) * stepScale;
        float percent = 1.0 - (float(i) + offset) / float(samplerRadius);

        {
            vec4 sampleColor1 = sampleImage(v_texCoord + dis);
            float distanceFromCentralColor1 = min(distance(centralColor, sampleColor1) * factor, 1.0);
            float gaussianWeight1 = percent * (1.0 - distanceFromCentralColor1);
            gaussianWeightTotal += gaussianWeight1;
            sum += sampleColor1 * gaussianWeight1;
        }

        {
            vec4 sampleColor2 = sampleImage(v_texCoord - dis);
            float distanceFromCentralColor2 = min(distance(centralColor, sampleColor2) * factor, 1.0);
            float gaussianWeight2 = percent * (1.0 - distanceFromCentralColor2);
            gaussianWeightTotal += gaussianWeight2;
            sum += sampleColor2 * gaussianWeight2;
        }
    }

    outColor = sum / gaussianWeightTotal;
}

