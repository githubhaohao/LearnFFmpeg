package com.byteflow.learnffmpeg.media;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class FFMediaRecorder extends MediaRecorderContext implements GLSurfaceView.Renderer {
    private static final String TAG = "CameraRender";
    private GLSurfaceView mGLSurfaceView;

    public FFMediaRecorder() {
    }

    public void init(GLSurfaceView surfaceView) { //for Video
        mGLSurfaceView = surfaceView;
        mGLSurfaceView.setEGLContextClientVersion(2);
        mGLSurfaceView.setRenderer(this);
        mGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        native_CreateContext();
        native_Init();
    }

    public void init() { //for audio
        native_CreateContext();
        native_Init();
    }

    public void requestRender() {
        if (mGLSurfaceView != null) {
            mGLSurfaceView.requestRender();
        }
    }

    public void setTransformMatrix(int degree, int mirror) {
        Log.d(TAG, "setTransformMatrix() called with: degree = [" + degree + "], mirror = [" + mirror + "]");
        native_SetTransformMatrix(0, 0, 1, 1, degree, mirror);
    }

    public void startRecord(int recorderType, String outUrl, int frameWidth, int frameHeight, long videoBitRate, int fps) {
        Log.d(TAG, "startRecord() called with: recorderType = [" + recorderType + "], outUrl = [" + outUrl + "], frameWidth = [" + frameWidth + "], frameHeight = [" + frameHeight + "], videoBitRate = [" + videoBitRate + "], fps = [" + fps + "]");
        native_StartRecord(recorderType, outUrl, frameWidth, frameHeight, videoBitRate, fps);
    }

    public void onPreviewFrame(int format, byte[] data, int width, int height) {
        Log.d(TAG, "onPreviewFrame() called with: data = [" + data + "], width = [" + width + "], height = [" + height + "]");
        native_OnPreviewFrame(format, data, width, height);
    }

    public void onAudioData(byte[] data, int size) {
        Log.d(TAG, "onAudioData() called with: data = [" + data + "], size = [" + size + "]");
        native_OnAudioData(data, size);
    }

    public void stopRecord() {
        Log.d(TAG, "stopRecord() called");
        native_StopRecord();
    }

    public void loadShaderFromAssetsFile(int shaderIndex, Resources r) {
        String result = null;
        try {
            InputStream in = r.getAssets().open("shaders/fshader_" + shaderIndex + ".glsl");
            int ch = 0;
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            while ((ch = in.read()) != -1) {
                baos.write(ch);
            }
            byte[] buff = baos.toByteArray();
            baos.close();
            in.close();
            result = new String(buff, "UTF-8");
            result = result.replaceAll("\\r\\n", "\n");
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (result != null) {
            setFragShader(shaderIndex, result);
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.d(TAG, "onSurfaceCreated() called with: gl = [" + gl + "], config = [" + config + "]");
        native_OnSurfaceCreated();

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.d(TAG, "onSurfaceChanged() called with: gl = [" + gl + "], width = [" + width + "], height = [" + height + "]");
        native_OnSurfaceChanged(width, height);

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Log.d(TAG, "onDrawFrame() called with: gl = [" + gl + "]");
        native_OnDrawFrame();
    }

    public void setFilterData(int index, int format, int width, int height, byte[] bytes) {
        native_SetFilterData(index, format, width, height, bytes);
    }

    public void setFragShader(int index, String str) {
        native_SetFragShader(index, str);
    }

    public void unInit() {
        native_UnInit();
        native_DestroyContext();
    }
}
