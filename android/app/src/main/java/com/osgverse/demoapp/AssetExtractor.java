package com.osgverse.demoapp;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Unpacks the assets of the APK to the application directory.
 *
 * OSG and osgVerse read their files (shaders, models, skyboxes) with the normal file API,
 * which cannot look inside an APK. The assets are therefore copied to a real directory once
 * and the native side uses that directory as its base directory (see main.cpp).
 */
final class AssetExtractor {
    private static final String TAG = "osgVerse";
    private static final String Folder = "assets";
    private static final String VersionFile = ".version";

    /** Bump this value whenever the assets change, so that they are unpacked again */
    private static final int Version = 2;

    static void extract(Context context) {
        File targetDir = new File(context.getFilesDir(), Folder);
        File versionFile = new File(targetDir, VersionFile);
        if (readVersion(versionFile) == Version) {
            Log.v(TAG, "Assets are already unpacked in " + targetDir);
            return;
        }

        Log.v(TAG, "Unpacking the assets to " + targetDir + " (only done once)...");
        long beginTime = System.currentTimeMillis();
        try {
            copyFolder(context.getAssets(), "", targetDir);
            writeVersion(versionFile);
            Log.i(TAG, "Unpacked the assets in " + (System.currentTimeMillis() - beginTime) + " ms");
        } catch (IOException e) {
            Log.e(TAG, "Failed to unpack the assets: " + e);
        }
    }

    private static void copyFolder(AssetManager assets, String assetPath, File targetDir)
            throws IOException {
        String[] children = assets.list(assetPath);
        if (children == null) return;
        if (children.length == 0) {
            copyFile(assets, assetPath, targetDir);
            return;
        }

        if (!targetDir.exists() && !targetDir.mkdirs())
            throw new IOException("Unable to create " + targetDir);

        for (String child : children) {
            String childPath = assetPath.isEmpty() ? child : (assetPath + "/" + child);
            copyFolder(assets, childPath, new File(targetDir, child));
        }
    }

    private static void copyFile(AssetManager assets, String assetPath, File targetFile)
            throws IOException {
        try (InputStream in = assets.open(assetPath);
             OutputStream out = new FileOutputStream(targetFile)) {
            byte[] buffer = new byte[64 * 1024];
            int count;
            while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
        }
    }

    private static int readVersion(File versionFile) {
        if (!versionFile.isFile()) return -1;
        try (InputStream in = new FileInputStream(versionFile)) {
            byte[] buffer = new byte[16];
            int count = in.read(buffer);
            return (count <= 0) ? -1 : Integer.parseInt(new String(buffer, 0, count).trim());
        } catch (IOException | NumberFormatException e) {
            return -1;
        }
    }

    private static void writeVersion(File versionFile) {
        try (OutputStream out = new FileOutputStream(versionFile)) {
            out.write(Integer.toString(Version).getBytes());
        } catch (IOException e) {
            Log.w(TAG, "Failed to write " + versionFile + ": " + e);
        }
    }
}
