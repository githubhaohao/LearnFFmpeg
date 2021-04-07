/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

package com.byteflow.learnffmpeg;

import android.Manifest;
import android.content.pm.PackageManager;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.SeekBar;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.byteflow.learnffmpeg.media.FFMediaPlayer;
import com.byteflow.learnffmpeg.media.MyGLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static com.byteflow.learnffmpeg.media.FFMediaPlayer.AUDIO_GL_RENDER;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_DURATION;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_HEIGHT;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_WIDTH;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MSG_DECODER_DONE;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MSG_DECODER_INIT_ERROR;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MSG_DECODER_READY;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MSG_DECODING_TIME;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.MSG_REQUEST_RENDER;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.VIDEO_GL_RENDER;
import static com.byteflow.learnffmpeg.media.FFMediaPlayer.VIDEO_RENDER_OPENGL;

public class AudioVisualMediaPlayerActivity extends AppCompatActivity implements GLSurfaceView.Renderer, FFMediaPlayer.EventCallback{
    private static final String TAG = "MediaPlayerActivity";
    private static final String[] REQUEST_PERMISSIONS = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };
    private static final int PERMISSION_REQUEST_CODE = 1;
    private MyGLSurfaceView mVideoGLSurfaceView = null;
    private MyGLSurfaceView mAudioGLSurfaceView = null;
    private FFMediaPlayer mMediaPlayer = null;
    private SeekBar mSeekBar = null;
    private boolean mIsTouch = false;
    private String mVideoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/byteflow/midway.mp4";

    private GLSurfaceView.Renderer mAudioGLRender = new GLSurfaceView.Renderer() {
        @Override
        public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
            FFMediaPlayer.native_OnSurfaceCreated(AUDIO_GL_RENDER);

        }

        @Override
        public void onSurfaceChanged(GL10 gl10, int w, int h) {
            FFMediaPlayer.native_OnSurfaceChanged(AUDIO_GL_RENDER, w, h);

        }

        @Override
        public void onDrawFrame(GL10 gl10) {
            FFMediaPlayer.native_OnDrawFrame(AUDIO_GL_RENDER);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_visual_media_player);

        mVideoGLSurfaceView = findViewById(R.id.video_surface_view);
        mVideoGLSurfaceView.setEGLContextClientVersion(3);
        mVideoGLSurfaceView.setRenderer(this);
        mVideoGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        mAudioGLSurfaceView = findViewById(R.id.audio_surface_view);
        mAudioGLSurfaceView.setEGLContextClientVersion(3);
        mAudioGLSurfaceView.setRenderer(mAudioGLRender);
        mAudioGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        mSeekBar = findViewById(R.id.seek_bar);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                mIsTouch = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                Log.d(TAG, "onStopTrackingTouch() called with: progress = [" + seekBar.getProgress() + "]");
                if(mMediaPlayer != null) {
                    mMediaPlayer.seekToPosition(mSeekBar.getProgress());
                    mIsTouch = false;
                }

            }
        });
        mMediaPlayer = new FFMediaPlayer();
        mMediaPlayer.addEventCallback(this);
        mMediaPlayer.init(mVideoPath, VIDEO_RENDER_OPENGL, null);
    }

    @Override
    protected void onResume() {
        Log.e(TAG, "onResume() called");
        super.onResume();
        if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
            ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, PERMISSION_REQUEST_CODE);
        } else {
            if(mMediaPlayer != null)
                mMediaPlayer.play();
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
                Toast.makeText(this, "We need the permission: WRITE_EXTERNAL_STORAGE", Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(mMediaPlayer != null)
            mMediaPlayer.pause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mMediaPlayer != null)
            mMediaPlayer.unInit();
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        FFMediaPlayer.native_OnSurfaceCreated(VIDEO_GL_RENDER);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int w, int h) {
        Log.d(TAG, "onSurfaceChanged() called with: gl10 = [" + gl10 + "], w = [" + w + "], h = [" + h + "]");
        FFMediaPlayer.native_OnSurfaceChanged(VIDEO_GL_RENDER, w, h);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        FFMediaPlayer.native_OnDrawFrame(VIDEO_GL_RENDER);
    }

    @Override
    public void onPlayerEvent(final int msgType, final float msgValue) {
        Log.d(TAG, "onPlayerEvent() called with: msgType = [" + msgType + "], msgValue = [" + msgValue + "]");
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                switch (msgType) {
                    case MSG_DECODER_INIT_ERROR:
                        break;
                    case MSG_DECODER_READY:
                        onDecoderReady();
                        break;
                    case MSG_DECODER_DONE:
                        break;
                    case MSG_REQUEST_RENDER:
                        mVideoGLSurfaceView.requestRender();
                        mAudioGLSurfaceView.requestRender();
                        break;
                    case MSG_DECODING_TIME:
                        if(!mIsTouch)
                            mSeekBar.setProgress((int) msgValue);
                        break;
                    default:
                        break;
                }
            }
        });

    }

    private void onDecoderReady() {
        int videoWidth = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_WIDTH);
        int videoHeight = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_HEIGHT);
        if(videoHeight * videoWidth != 0)
            mVideoGLSurfaceView.setAspectRatio(videoWidth, videoHeight);

        int duration = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_DURATION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mSeekBar.setMin(0);
        }
        mSeekBar.setMax(duration);
    }

    protected boolean hasPermissionsGranted(String[] permissions) {
        for (String permission : permissions) {
            if (ActivityCompat.checkSelfPermission(this, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

}
