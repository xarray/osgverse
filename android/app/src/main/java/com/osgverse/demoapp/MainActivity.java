package com.osgverse.demoapp;

import android.content.Intent;
import android.os.Bundle;
import android.app.Activity;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {
    private Button _sdlLaunchButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // The assets have to be unpacked to the application directory before anything is
        // rendered (see AssetExtractor), which takes a moment when the application runs for
        // the first time. It is done in the background to keep the user interface responsive,
        // and the button stays disabled until the files are complete
        _sdlLaunchButton = findViewById(R.id.sdlLaunchButton);
        _sdlLaunchButton.setEnabled(false);
        _sdlLaunchButton.setText(R.string.preparing_assets);
        new Thread(new Runnable() {
            public void run() {
                AssetExtractor.extract(MainActivity.this);
                runOnUiThread(new Runnable() {
                    public void run() {
                        _sdlLaunchButton.setText(R.string.launch_sdl_demo);
                        _sdlLaunchButton.setEnabled(true);
                    }
                });
            }
        }, "AssetExtractor").start();
    }

    public void startSDLActivity(View view) {
        Intent intent = new Intent(this, OsgVerseSDLActivity.class);
        startActivity(intent);
    }

}
