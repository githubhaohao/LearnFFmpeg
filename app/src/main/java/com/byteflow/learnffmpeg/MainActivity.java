package com.byteflow.learnffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.byteflow.learnffmpeg.media.FFMediaPlayer;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ((TextView)findViewById(R.id.text_view)).setText(FFMediaPlayer.GetFFmpegVersion());
    }
}
