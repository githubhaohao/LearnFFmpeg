package com.byteflow.learnffmpeg.media;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.util.Log;

public class AudioRecorder extends Thread {
	private static final String TAG = "AudioRecorder";
	private AudioRecord mAudioRecord = null;
	private static final int DEFAULT_SAMPLE_RATE = 44100;
	private static final int DEFAULT_CHANNEL_LAYOUT = AudioFormat.CHANNEL_IN_STEREO;
	private static final int DEFAULT_SAMPLE_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
	private final AudioRecorderCallback mRecorderCallback;

	public AudioRecorder(AudioRecorderCallback callback) {
		this.mRecorderCallback = callback;
	}

	@Override
	public void run() {

		final int mMinBufferSize = AudioRecord.getMinBufferSize(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_LAYOUT, DEFAULT_SAMPLE_FORMAT);
		Log.d(TAG, "run() called mMinBufferSize=" + mMinBufferSize);

		if (AudioRecord.ERROR_BAD_VALUE == mMinBufferSize) {
			mRecorderCallback.onError("parameters are not supported by the hardware.");
			return;
		}

		mAudioRecord = new AudioRecord(android.media.MediaRecorder.AudioSource.MIC, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_LAYOUT, DEFAULT_SAMPLE_FORMAT, mMinBufferSize);
		try {
			mAudioRecord.startRecording();
		} catch (IllegalStateException e) {
			mRecorderCallback.onError(e.getMessage() + " [startRecording failed]");
			return;
		}

		byte[] sampleBuffer = new byte[4096];
		try {
			while (!Thread.currentThread().isInterrupted()) {

				int result = mAudioRecord.read(sampleBuffer, 0, 4096);
				if (result > 0) {
					mRecorderCallback.onAudioData(sampleBuffer, result);
				}
			}
		} catch (Exception e) {
			mRecorderCallback.onError(e.getMessage());
		}

		mAudioRecord.release();
		mAudioRecord = null;
	}

	public interface AudioRecorderCallback {
		void onAudioData(byte[] data, int dataSize);
		void onError(String msg);
	}
}
