package com.byteflow.learnffmpeg.view;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;

public class MyGestureListener extends GestureDetector.SimpleOnGestureListener {

    public enum SwipeDirection {
        SWIPE_UP, SWIPE_DOWN, SWIPE_LEFT, SWIPE_RIGHT
    }

    private static final int SWIPE_MIN_DISTANCE = 100;
    private static final int SWIPE_THRESHOLD_VELOCITY = 100;

    private GestureDetector mGestureDetector;
    private SimpleGestureListener mGestureListener;

    public MyGestureListener(Context context, SimpleGestureListener listener) {
        mGestureDetector = new GestureDetector(context, this);
        mGestureListener = listener;
    }

    public void onTouchEvent(MotionEvent event) {
        mGestureDetector.onTouchEvent(event);
    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        if (e1.getX() - e2.getX() > SWIPE_MIN_DISTANCE && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
            mGestureListener.onSwipe(SwipeDirection.SWIPE_RIGHT);
            return true;
        } else if (e2.getX() - e1.getX() > SWIPE_MIN_DISTANCE && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
            mGestureListener.onSwipe(SwipeDirection.SWIPE_LEFT);
            return true;
        }

        if (e1.getY() - e2.getY() > SWIPE_MIN_DISTANCE && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
            mGestureListener.onSwipe(SwipeDirection.SWIPE_UP);
            return true;
        } else if (e2.getY() - e1.getY() > SWIPE_MIN_DISTANCE && Math.abs(velocityY) > SWIPE_THRESHOLD_VELOCITY) {
            mGestureListener.onSwipe(SwipeDirection.SWIPE_DOWN);
            return true;
        }
        return false;
    }

    public interface SimpleGestureListener {
        void onSwipe(SwipeDirection direction);
    }
}
