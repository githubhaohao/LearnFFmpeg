/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

package com.byteflow.learnffmpeg.camera;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.AndroidRuntimeException;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class Camera2Wrapper {
    private static final String TAG = "Camera2Wrapper";
    private static final int DEFAULT_CAMERA_ID = 0;
    private final float THRESHOLD = 0.001f;

    private Camera2FrameCallback mCamera2FrameCallback;
    private Context mContext;
    private CameraManager mCameraManager;

    private CameraCaptureSession mCameraCaptureSession;
    private CaptureRequest mPreviewRequest;
    private CameraDevice mCameraDevice;
    private String mCameraId;
    private String[] mSupportCameraIds;
    private ImageReader mPreviewImageReader, mCaptureImageReader;
    private Integer mSensorOrientation;

    private Semaphore mCameraLock = new Semaphore(1);
    private Size mDefaultPreviewSize = new Size(1280, 720);
    private Size mDefaultCaptureSize = new Size(1280, 720);

    private Surface mPreviewSurface;

    private Size mPreviewSize, mPictureSize;
    private List<Size> mSupportPreviewSize, mSupportPictureSize;

    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;

    private ImageReader.OnImageAvailableListener mOnPreviewImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = reader.acquireLatestImage();
            if (image != null) {
                if (mCamera2FrameCallback != null) {
                    mCamera2FrameCallback.onPreviewFrame(CameraUtil.YUV_420_888_data(image), image.getWidth(), image.getHeight());
                }
                image.close();
            }
        }
    };

    private ImageReader.OnImageAvailableListener mOnCaptureImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = reader.acquireLatestImage();
            if (image != null) {
                if (mCamera2FrameCallback != null) {
                    mCamera2FrameCallback.onCaptureFrame(CameraUtil.YUV_420_888_data(image), image.getWidth(), image.getHeight());
                }
                image.close();
            }
        }
    };

    public Camera2Wrapper(Context context) {
        mContext = context;
        mCamera2FrameCallback = (Camera2FrameCallback) context;
        initCamera2Wrapper();
    }

    public Camera2Wrapper(Context context, Camera2FrameCallback callback) {
        mContext = context;
        mCamera2FrameCallback = callback;
        initCamera2Wrapper();
    }

    private void initCamera2Wrapper() {
        mCameraManager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            mSupportCameraIds = mCameraManager.getCameraIdList();
            if (checkCameraIdSupport(String.valueOf(DEFAULT_CAMERA_ID))) {
            } else {
                throw new AndroidRuntimeException("Don't support the camera id: " + DEFAULT_CAMERA_ID);
            }
            mCameraId = String.valueOf(DEFAULT_CAMERA_ID);
            getCameraInfo(mCameraId);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void getCameraInfo(String cameraId) {
        CameraCharacteristics characteristics = null;
        try {
            characteristics = mCameraManager.getCameraCharacteristics(cameraId);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

        StreamConfigurationMap streamConfigs = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        if (streamConfigs != null) {
            mSupportPreviewSize = Arrays.asList(streamConfigs.getOutputSizes(SurfaceTexture.class));

            boolean supportDefaultSize = false;
            Size sameRatioSize = null;
            float defaultRatio = mDefaultPreviewSize.getWidth() * 1.0f / mDefaultPreviewSize.getHeight();
            mPreviewSize = mSupportPreviewSize.get(mSupportPreviewSize.size() / 2);
            for (Size size : mSupportPreviewSize) {
                Log.d(TAG, "initCamera2Wrapper() called mSupportPreviewSize " + size.getWidth() + "x" + size.getHeight());
                float ratio = size.getWidth() * 1.0f / size.getHeight();
                if (Math.abs(ratio - defaultRatio) < THRESHOLD) {
                    Log.d(TAG, "initCamera2Wrapper() called mSupportPreviewSize sameRatioSize == size" + size.getWidth() + "x" + size.getHeight());
                    sameRatioSize = size;
                }

                if (mDefaultPreviewSize.getWidth() == size.getWidth() && mDefaultPreviewSize.getHeight() == size.getHeight()) {
                    Log.d(TAG, "initCamera2Wrapper() called supportDefaultSize ");
                    supportDefaultSize = true;
                    break;
                }
            }

            if (supportDefaultSize) {
                mPreviewSize = mDefaultPreviewSize;
            } else if(sameRatioSize != null) {
                mPreviewSize = sameRatioSize;
            }
            //Log.d(TAG, "initCamera2Wrapper() called mPreviewSize[w,h] = [" + mPreviewSize.getWidth() + "," + mPictureSize.getHeight() + "]");

            supportDefaultSize = false;
            sameRatioSize = null;
            defaultRatio = mDefaultCaptureSize.getWidth() * 1.0f / mDefaultCaptureSize.getHeight();
            mSupportPictureSize = Arrays.asList(streamConfigs.getOutputSizes(ImageFormat.YUV_420_888));
            mPictureSize = mSupportPictureSize.get(0);
            for (Size size : mSupportPictureSize) {
                Log.d(TAG, "initCamera2Wrapper() called mSupportPictureSize " + size.getWidth() + "x" + size.getHeight());
                float ratio = size.getWidth() * 1.0f / size.getHeight();
                if (Math.abs(ratio - defaultRatio) < THRESHOLD) {
                    sameRatioSize = size;
                }

                if (mDefaultCaptureSize.getWidth() == size.getWidth() && mDefaultCaptureSize.getHeight() == size.getHeight()) {
                    supportDefaultSize = true;
                    break;
                }
            }
            if (supportDefaultSize) {
                mPictureSize = mDefaultCaptureSize;
            }  else if(sameRatioSize != null) {
                mPictureSize = sameRatioSize;
            }
        }
        mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
        Log.d(TAG, "initCamera2Wrapper() called mSensorOrientation = " + mSensorOrientation);

    }

    private boolean checkCameraIdSupport(String cameraId) {
        boolean isSupported = false;
        for (String id: mSupportCameraIds) {
            if (cameraId.equals(id)) {
                isSupported = true;
            }
        }
        return isSupported;
    }

    public void startCamera() {
        startBackgroundThread();
        if (mPreviewImageReader == null && mPreviewSize != null) {
            mPreviewImageReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 2);
            mPreviewImageReader.setOnImageAvailableListener(mOnPreviewImageAvailableListener, mBackgroundHandler);
            mPreviewSurface = mPreviewImageReader.getSurface();
        }

        if (mCaptureImageReader == null && mPictureSize != null) {
            mCaptureImageReader = ImageReader.newInstance(mPictureSize.getWidth(), mPictureSize.getHeight(), ImageFormat.YUV_420_888, 2);
            mCaptureImageReader.setOnImageAvailableListener(mOnCaptureImageAvailableListener, mBackgroundHandler);
        }

        openCamera();
    }

    public void startCamera(SurfaceTexture previewSurfaceTex) {
        startBackgroundThread();
        if (previewSurfaceTex != null) {
            previewSurfaceTex.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
            mPreviewSurface = new Surface(previewSurfaceTex);
        } else {
            mPreviewImageReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 2);
            mPreviewImageReader.setOnImageAvailableListener(mOnPreviewImageAvailableListener, mBackgroundHandler);
            mPreviewSurface = mPreviewImageReader.getSurface();
        }

        if (mCaptureImageReader == null && mPictureSize != null) {
            mCaptureImageReader = ImageReader.newInstance(mPictureSize.getWidth(), mPictureSize.getHeight(), ImageFormat.YUV_420_888, 2);
            mCaptureImageReader.setOnImageAvailableListener(mOnCaptureImageAvailableListener, mBackgroundHandler);
        }

        openCamera();
    }

    public void stopCamera() {
        if (mPreviewImageReader != null) {
            mPreviewImageReader.setOnImageAvailableListener(null, null);
        }

        if (mCaptureImageReader != null) {
            mCaptureImageReader.setOnImageAvailableListener(null, null);
        }
        closeCamera();
        stopBackgroundThread();
    }

    public void updatePreviewSize(Size size) {
        if (size != null && !size.equals(mPreviewSize)) {
            mPreviewSize = size;
            stopCamera();
            startCamera();
        }
    }

    public void updatePictureSize(Size size) {
        if (size != null && !size.equals(mPictureSize)) {
            mPictureSize = size;
            stopCamera();
            startCamera();
        }
    }

    public void updateCameraId(String cameraId) {
        for (String id : mSupportCameraIds) {
            if (id.equals(cameraId) && !mCameraId.equals(cameraId)) {
                mCameraId = cameraId;
                getCameraInfo(mCameraId);
                stopCamera();
                startCamera();
                break;
            }
        }
    }

    private void openCamera() {
        Log.d(TAG, "openCamera() called");
        if (ContextCompat.checkSelfPermission(mContext, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            return;
        }

        CameraManager manager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            if (!mCameraLock.tryAcquire(2500, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }
            manager.openCamera(mCameraId, mStateCallback, mBackgroundHandler);
        } catch (CameraAccessException e) {
            Log.e(TAG, "Cannot access the camera." + e.toString());
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.", e);
        }
    }

    public void closeCamera() {
        Log.d(TAG, "closeCamera() called");
        try {
            mCameraLock.acquire();
            if (null != mCameraCaptureSession) {
                mCameraCaptureSession.close();
                mCameraCaptureSession = null;
            }
            if (null != mCameraDevice) {
                mCameraDevice.close();
                mCameraDevice = null;
            }
            if (null != mPreviewImageReader) {
                mPreviewImageReader.close();
                mPreviewImageReader = null;
            }

            if (null != mCaptureImageReader) {
                mCaptureImageReader.close();
                mCaptureImageReader = null;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera closing.", e);
        } finally {
            mCameraLock.release();
        }

    }

    private final CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {

        @Override
        public void onOpened(@NonNull CameraDevice cameraDevice) {
            // This method is called when the camera is opened.  We start camera preview here.
            mCameraLock.release();
            mCameraDevice = cameraDevice;
            createCaptureSession();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            mCameraLock.release();
            cameraDevice.close();
            mCameraDevice = null;
        }

        @Override
        public void onError(@NonNull CameraDevice cameraDevice, int error) {
            mCameraLock.release();
            cameraDevice.close();
            mCameraDevice = null;
        }
    };

    private void createCaptureSession() {
        try {
            if (null == mCameraDevice || null == mPreviewSurface || null == mCaptureImageReader) return;
            mCameraDevice.createCaptureSession(Arrays.asList(mPreviewSurface, mCaptureImageReader.getSurface()),
                    mSessionStateCallback, mBackgroundHandler);

        } catch (CameraAccessException e) {
            Log.e(TAG, "createCaptureSession " + e.toString());
        }
    }

    private CameraCaptureSession.StateCallback mSessionStateCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onConfigured(@NonNull CameraCaptureSession session) {
            mCameraCaptureSession = session;
            try {
                mPreviewRequest = createPreviewRequest();
                if (mPreviewRequest != null) {
                    session.setRepeatingRequest(mPreviewRequest, null, mBackgroundHandler);
                } else {
                    Log.e(TAG, "captureRequest is null");
                }
            } catch (CameraAccessException e) {
                Log.e(TAG, "onConfigured " + e.toString());
            }
        }

        @Override
        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
            Log.e(TAG, "onConfigureFailed");
        }
    };

    private CaptureRequest createPreviewRequest() {
        if (null == mCameraDevice || mPreviewSurface == null) return null;
        try {
            CaptureRequest.Builder builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            builder.addTarget(mPreviewSurface);
            return builder.build();
        } catch (CameraAccessException e) {
            Log.e(TAG, e.getMessage());
            return null;
        }
    }

    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("Camera2Background");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    private void stopBackgroundThread() {
        if (mBackgroundThread != null) {
            mBackgroundThread.quitSafely();
            try {
                mBackgroundThread.join();
                mBackgroundThread = null;
                mBackgroundHandler = null;
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void capture() {
        if (mCameraDevice == null) return;
        final CaptureRequest.Builder captureBuilder;
        try {
            captureBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureBuilder.addTarget(mCaptureImageReader.getSurface());

            // Use the same AE and AF modes as the preview.
            captureBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                    CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);

            // Orientation
            CameraCaptureSession.CaptureCallback CaptureCallback
                    = new CameraCaptureSession.CaptureCallback() {

                @Override
                public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                                               @NonNull CaptureRequest request,
                                               @NonNull TotalCaptureResult result) {
                    if (mPreviewRequest != null && mCameraCaptureSession != null) {
                        try {
                            mCameraCaptureSession.setRepeatingRequest(mPreviewRequest, null, mBackgroundHandler);
                        } catch (CameraAccessException e) {
                            e.printStackTrace();
                        }
                    }
                }

            };

            mCameraCaptureSession.stopRepeating();
            mCameraCaptureSession.abortCaptures();
            mCameraCaptureSession.capture(captureBuilder.build(), CaptureCallback, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void setDefaultPreviewSize(Size size) {
        if (size != null && size.getWidth() * size.getHeight() > 0) {
            mDefaultPreviewSize = size;
            getCameraInfo(mCameraId);
        }
    }

    public void setDefaultCaptureSize(Size size) {
        if (size != null && size.getWidth() * size.getHeight() > 0) {
            mDefaultCaptureSize = size;
            getCameraInfo(mCameraId);
        }
    }

    public String getCameraId() {
        return mCameraId;
    }

    public String[] getSupportCameraIds() {
        return mSupportCameraIds;
    }

    public List<Size> getSupportPreviewSize() {
        return mSupportPreviewSize;
    }

    public List<Size> getSupportPictureSize() {
        return mSupportPictureSize;
    }

    public Size getPreviewSize() {
        return mPreviewSize;
    }

    public Size getPictureSize() {
        return mPictureSize;
    }

    public Integer getSensorOrientation() {
        return mSensorOrientation;
    }
}
