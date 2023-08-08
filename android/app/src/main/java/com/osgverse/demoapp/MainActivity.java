package com.osgverse.demoapp;

import android.content.Intent;
import android.os.Bundle;
import android.app.Activity;
import android.view.View;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void startSDLActivity(View view) {
        Intent intent = new Intent(this, SDLActivity.class);
        startActivity(intent);
    }

}
