package com.ifstile;

import android.content.ContentResolver;
//import android.content.pm.ActivityInfo;
import android.content.Intent;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.os.ParcelFileDescriptor;
import java.io.FileNotFoundException;


import org.libsdl.app.SDLActivity;

public class IFStile extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[]{"IFStile"};
    }

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint){}
	
	static final int OPEN_FILE_REQUEST = 0;
    static final int CREATE_FILE_REQUEST = 1;

    public static void chooseFile(int requestCode, String filename) {
        if (requestCode == OPEN_FILE_REQUEST){
            Intent openFile = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            openFile.setType("*/*");
            openFile.addCategory(Intent.CATEGORY_OPENABLE);
            mSingleton.startActivityForResult(openFile, requestCode);
        }else if (requestCode == CREATE_FILE_REQUEST){
            Intent openFile = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            openFile.setType("*/*");
            openFile.addCategory(Intent.CATEGORY_OPENABLE);
            openFile.putExtra(Intent.EXTRA_TITLE, filename);
            mSingleton.startActivityForResult(openFile, requestCode);
        }
    }

    public static native void nativeOnFDReady(int fd, int requestCode, String uri);

    private static String queryName(ContentResolver resolver, Uri uri) {
        Cursor returnCursor =
                resolver.query(uri, null, null, null, null);
        int nameIndex = returnCursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
        returnCursor.moveToFirst();
        String name = returnCursor.getString(nameIndex);
        returnCursor.close();
        return name;
    }
   
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode != RESULT_OK)return;

        String mode;
        if (requestCode == OPEN_FILE_REQUEST) {
            mode="r";
        }else if (requestCode == CREATE_FILE_REQUEST) {
            mode="w";
        }else {
            return;
        }

        Uri uri = data.getData();
        ContentResolver res = getContentResolver();

        ParcelFileDescriptor pfd;
        try {
            pfd = res.openFileDescriptor(uri, mode);
        }catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        }

        int file = pfd.detachFd();
        String fileName = queryName(res, uri);
        nativeOnFDReady(file, requestCode, fileName);
    }
	
	/*
	private static native void nativeSetAssetMgr(AssetManager mgr);

	private AssetManager AssetMgr;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
        AssetMgr = getResources().getAssets();
		nativeSetAssetMgr(AssetMgr);
	}
	*/
}
