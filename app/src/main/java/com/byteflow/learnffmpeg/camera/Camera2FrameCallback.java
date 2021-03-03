package com.byteflow.learnffmpeg.camera;

public interface Camera2FrameCallback {
    void onPreviewFrame(byte[] data, int width, int height);
    void onCaptureFrame(byte[] data, int width, int height);
}
