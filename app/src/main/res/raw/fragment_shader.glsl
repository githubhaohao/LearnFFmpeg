#extension GL_OES_EGL_image_external : require //申明使用扩展纹理
precision mediump float;//精度 为float
varying vec2 v_texPo;//纹理位置  接收于vertex_shader
uniform samplerExternalOES  sTexture;//加载流数据(摄像头数据)
void main() {
  vec4 tempColor = texture2D(sTexture, v_texPo);
  float luminance = tempColor.r * 0.299 + tempColor.g * 0.587 + tempColor.b * 0.114;
  gl_FragColor = vec4(vec3(luminance), tempColor.a);
}