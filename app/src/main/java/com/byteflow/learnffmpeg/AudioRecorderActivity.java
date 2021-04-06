/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

package com.byteflow.learnffmpeg;

import android.Manifest;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraCharacteristics;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.FileProvider;

import com.byteflow.learnffmpeg.media.AudioRecorder;
import com.byteflow.learnffmpeg.media.FFMediaRecorder;
import com.byteflow.learnffmpeg.view.CaptureLayout;
import com.byteflow.learnffmpeg.view.CaptureListener;
import com.byteflow.learnffmpeg.view.TypeListener;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;

import static com.byteflow.learnffmpeg.media.MediaRecorderContext.RECORDER_TYPE_SINGLE_AUDIO;
import static com.byteflow.learnffmpeg.view.RecordedButton.BUTTON_STATE_ONLY_RECORDER;


public class AudioRecorderActivity extends AppCompatActivity implements AudioRecorder.AudioRecorderCallback {
    private static final String TAG = "MainActivity";
    private static final SimpleDateFormat DateTime_FORMAT = new SimpleDateFormat("yyyyMMddHHmmss", Locale.US);
    private static final String RESULT_IMG_DIR = "byteflow/learnffmpeg";
    private static final String[] REQUEST_PERMISSIONS = {
            Manifest.permission.CAMERA,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };
    private static final int CAMERA_PERMISSION_REQUEST_CODE = 1;
    protected FFMediaRecorder mFFMediaRecorder;
    private CaptureLayout mRecordedButton;
    private AudioRecorder mAudioRecorder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        requestWindowFeature(Window.FEATURE_NO_TITLE);
//        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
//                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_audio_recorder);
        mFFMediaRecorder = new FFMediaRecorder();
        mFFMediaRecorder.init();

        initViews();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
            ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, CAMERA_PERMISSION_REQUEST_CODE);
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == CAMERA_PERMISSION_REQUEST_CODE) {
            if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
                Toast.makeText(this, "We need the camera permission.", Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mFFMediaRecorder.unInit();
    }

    private String mOutUrl;

    private void initViews() {
        mRecordedButton = findViewById(R.id.record_view);
        mRecordedButton.setButtonFeatures(BUTTON_STATE_ONLY_RECORDER);
        mRecordedButton.setDuration(20 * 1000);
        mRecordedButton.setIconSrc(0, 0);
        mRecordedButton.setCaptureLisenter(new CaptureListener() {
            @Override
            public void takePictures() {

            }

            @Override
            public void recordShort(long time) {
                ///mRecordedButton.setTextWithAnimation("录制时间过短");
            }

            @Override
            public void recordStart() {
                mOutUrl = getOutFile(".aac").getAbsolutePath();
                mFFMediaRecorder.startRecord(RECORDER_TYPE_SINGLE_AUDIO, mOutUrl, 0, 0, 0, 0);
                mAudioRecorder = new AudioRecorder(AudioRecorderActivity.this);
                mAudioRecorder.start();

            }

            @Override
            public void recordEnd(long time) {
                mAudioRecorder.interrupt();
                try {
                    mAudioRecorder.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                mAudioRecorder = null;
                final ProgressDialog progressDialog = ProgressDialog.show(AudioRecorderActivity.this, null, "[软编]等待队列中的数据编码完成", false, false);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        mFFMediaRecorder.stopRecord();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                progressDialog.dismiss();
                            }
                        });
                    }
                }).start();
                mRecordedButton.resetCaptureLayout();
                mRecordedButton.resetCaptureLayout();
            }

            @Override
            public void recordZoom(float zoom) {

            }

            @Override
            public void recordError() {
                mRecordedButton.resetCaptureLayout();

            }
        });

        //确认 取消
        mRecordedButton.setTypeLisenter(new TypeListener() {
            @Override
            public void cancel() {
                mRecordedButton.resetCaptureLayout();
            }

            @Override
            public void confirm() {
                mRecordedButton.resetCaptureLayout();
                Toast.makeText(AudioRecorderActivity.this, "音频已保存至：" + mOutUrl, Toast.LENGTH_SHORT).show();
                Intent intent = new Intent(Intent.ACTION_VIEW);
                File file = new File(mOutUrl);
                Uri uri = null;
                if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
                    uri = FileProvider.getUriForFile(AudioRecorderActivity.this,"com.byteflow.learnffmpeg.fileprovider", file);
                }else {
                    uri = Uri.fromFile(file);
                }
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                intent.setDataAndType(uri, "audio/*");
                startActivity(intent);
            }
        });
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

    public void updateTransformMatrix(String cameraId) {
        if (Integer.valueOf(cameraId) == CameraCharacteristics.LENS_FACING_FRONT) {
            mFFMediaRecorder.setTransformMatrix(90, 0);
        } else {
            mFFMediaRecorder.setTransformMatrix(90, 1);
        }

    }

    public static final File getOutFile(final String ext) {
        final File dir = new File(Environment.getExternalStorageDirectory(), RESULT_IMG_DIR);
        Log.d(TAG, "path=" + dir.toString());
        dir.mkdirs();
        if (dir.canWrite()) {
            return new File(dir, "audio_" + getDateTimeString() + ext);
        }
        return null;
    }

    private static final String getDateTimeString() {
        final GregorianCalendar now = new GregorianCalendar();
        return DateTime_FORMAT.format(now.getTime());
    }

    @Override
    public void onAudioData(byte[] data, int dataSize) {
        mFFMediaRecorder.onAudioData(data, dataSize);
    }

    @Override
    public void onError(final String msg) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(AudioRecorderActivity.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }
}
