package com.byteflow.learnffmpeg.media;

import android.view.Surface;

public class FFMediaPlayer {

    static {
        System.loadLibrary("learn-ffmpeg");
    }

    public static final int MSG_DECODER_INIT_ERROR      = 0;
    public static final int MSG_DECODER_READY           = 1;
    public static final int MSG_DECODER_DONE            = 2;
    public static final int MSG_REQUEST_RENDER          = 3;
    public static final int MSG_DECODING_TIME           = 4;

    public static final int MEDIA_PARAM_VIDEO_WIDTH     = 0x0001;
    public static final int MEDIA_PARAM_VIDEO_HEIGHT    = 0x0002;
    public static final int MEDIA_PARAM_VIDEO_DURATION  = 0x0003;

    private long mNativePlayerHandle = 0;

    private EventCallback mEventCallback = null;

    public static String GetFFmpegVersion() {
        return native_GetFFmpegVersion();
    }

    public void init(String url, Surface surface) {
        mNativePlayerHandle = native_Init(url, surface);
    }

    public void play() {
        native_Play(mNativePlayerHandle);
    }

    public void pause() {
        native_Pause(mNativePlayerHandle);
    }

    public void seekToPosition(float position) {
        native_SeekToPosition(mNativePlayerHandle, position);
    }

    public void stop() {
        native_Stop(mNativePlayerHandle);
    }

    public void unInit() {
        native_UnInit(mNativePlayerHandle);
    }

    public void addEventCallback(EventCallback callback) {
        mEventCallback = callback;
    }

    public long getMediaParams(int paramType) {
        return native_GetMediaParams(mNativePlayerHandle, paramType);
    }

    private void playerEventCallback(int msgType, float msgValue) {
        if(mEventCallback != null)
            mEventCallback.onPlayerEvent(msgType, msgValue);

    }

    private static native String native_GetFFmpegVersion();

    private native long native_Init(String url, Object surface);

    private native void native_Play(long playerHandle);

    private native void native_SeekToPosition(long playerHandle, float position);

    private native void native_Pause(long playerHandle);

    private native void native_Stop(long playerHandle);

    private native void native_UnInit(long playerHandle);

    private native long native_GetMediaParams(long playerHandle, int paramType);

    public interface EventCallback {
        void onPlayerEvent(int msgType, float msgValue);
    }

}
