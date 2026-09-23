package com.osgverse.demoapp;

import android.os.Bundle;

/**
 * The SDL activity used by the demo application.
 *
 * The Java classes of SDL (org.libsdl.app.*) are taken directly from the SDL2 source
 * tree that is used to build libSDL2.so (see java.srcDirs in build.gradle), so they
 * always match the native library. The names of the libraries to load are application
 * specific and are therefore set here.
 */
public class OsgVerseSDLActivity extends org.libsdl.app.SDLActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // The rendering is started by SDLActivity when the surface becomes ready, which
        // happens after onCreate() has returned, so the assets are ready before that
        AssetExtractor.extract(this);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            // The last entry is also used as the application entry point (see
            // SDLActivity.getMainSharedObject), so it has to be the bridge library
            "osgVerseAndroid"
        };
    }
}
