#version 300 es
precision highp float;
in vec2 v_texCoord;
layout(location = 0) out vec4 outColor;
uniform sampler2D s_texture0;
uniform sampler2D s_texture1;
uniform sampler2D s_texture2;
uniform sampler2D s_textureMapping;
uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420
uniform float u_Offset;
uniform vec2 u_TexSize;
uniform vec2 asciiTexSize;

/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * 字符画
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

const vec3  RGB2GRAY_VEC3 = vec3(0.299, 0.587, 0.114);
const float MESH_WIDTH = 16.0;//一个字符的宽
const float MESH_HEIGHT= 23.0;//一个字符的高
const float GARY_LEVEL = 24.0;//字符表图上有 24 个字符
const float ASCIIS_WIDTH = 8.0;//字符表列数
const float ASCIIS_HEIGHT = 3.0;//字符表行数
const float MESH_ROW_NUM = 100.0;//固定小格子的行数

void main()
{
    float imageMeshWidth = u_TexSize.x / MESH_ROW_NUM;
    float imageMeshHeight = imageMeshWidth * MESH_HEIGHT / MESH_WIDTH;

    vec2 imageTexCoord = v_texCoord * u_TexSize;//归一化坐标转像素坐标
    vec2 midTexCoord;
    midTexCoord.x = floor(imageTexCoord.x / imageMeshWidth) * imageMeshWidth + imageMeshWidth * 0.5;//小格子中心
    midTexCoord.y = floor(imageTexCoord.y / imageMeshHeight) * imageMeshHeight + imageMeshHeight * 0.5;//小格子中心

    vec2 normalizedTexCoord = midTexCoord / u_TexSize;//归一化
    vec4 rgbColor = sampleImage(normalizedTexCoord);//采样
    float grayValue = dot(rgbColor.rgb, RGB2GRAY_VEC3);//rgb分量转灰度值
    //outColor = vec4(vec3(grayValue), rgbColor.a);

    float offsetX = mod(imageTexCoord.x, imageMeshWidth) * MESH_WIDTH / imageMeshWidth;
    float offsetY = mod(imageTexCoord.y, imageMeshHeight) * MESH_HEIGHT / imageMeshHeight;

    float asciiIndex = floor((1.0 - grayValue) * GARY_LEVEL);//第几个字符
    float asciiIndexX = mod(asciiIndex, ASCIIS_WIDTH);
    float asciiIndexY = floor(asciiIndex / ASCIIS_WIDTH);

    vec2 grayTexCoord;
    grayTexCoord.x = (asciiIndexX * MESH_WIDTH + offsetX) / asciiTexSize.x;
    grayTexCoord.y = (asciiIndexY * MESH_HEIGHT + offsetY) / asciiTexSize.y;

    vec4 originColor = sampleImage(v_texCoord);//采样原始纹理
    vec4 mappingColor = vec4(texture(s_textureMapping, grayTexCoord).rgb, rgbColor.a);

    outColor = mix(originColor, mappingColor, u_Offset);
}

