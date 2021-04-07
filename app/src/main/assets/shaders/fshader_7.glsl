#version 300 es
precision highp float;
in vec2 v_texCoord;
layout(location = 0) out vec4 outColor;
uniform sampler2D s_texture0;
uniform sampler2D s_texture1;
uniform sampler2D s_texture2;
uniform sampler2D s_LutTexture;
uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420
uniform float u_Offset;
uniform vec2 u_TexSize;

/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * LUT滤镜
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

vec4 LutFilter(vec2 texCoord)
{
    //原始采样像素的 RGBA 值
    vec4 textureColor = sampleImage(texCoord);

    //获取 B 分量值，确定 LUT 小方格的 index, 取值范围转为 0～63
    float blueColor = textureColor.b * 63.0;

    //取与 B 分量值最接近的 2 个小方格的坐标
    vec2 quad1;
    quad1.y = floor(floor(blueColor) / 8.0);
    quad1.x = floor(blueColor) - (quad1.y * 8.0);

    vec2 quad2;
    quad2.y = floor(ceil(blueColor) / 7.9999);
    quad2.x = ceil(blueColor) - (quad2.y * 8.0);

    //通过 R 和 G 分量的值确定小方格内目标映射的 RGB 组合的坐标，然后归一化，转化为纹理坐标。
    vec2 texPos1;
    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

    vec2 texPos2;
    texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
    texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

    //取目标映射对应的像素值
    vec4 newColor1 = texture(s_LutTexture, texPos1);
    vec4 newColor2 = texture(s_LutTexture, texPos2);

    //使用 Mix 方法对 2 个边界像素值进行混合
    vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
    return mix(textureColor, vec4(newColor.rgb, textureColor.w), 1.0);
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
    if(v_texCoord.y > 0.5)
    {
        outColor = LutFilter(v_texCoord);
    }
    else
    {
        outColor = sampleImage(v_texCoord);
    }

}

