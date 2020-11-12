attribute vec4 av_Position;//顶点位置
attribute vec2 af_Position;//纹理位置
varying vec2 v_texPo;//纹理位置  与fragment_shader交互
uniform mat4 u_Matrix;//矩阵变换

void main() {
    v_texPo = af_Position;
    gl_Position = av_Position * u_Matrix;
}

