// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifdef __ANDROID__

#include "pch.h" 
#include "version.h"



#include <iosfwd>

#if 0
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

void set_thread_name(const char* ) 
{
	
}

void ext_prevent_sleep_mode(bool) 
{
	
}

void SDL_EnableWindow(SDL_Window*, bool)
{
	
};




void AndroidChooseFile(const std::string& filename, int is_save)
{
	// retrieve the JNI environment.
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();

	// retrieve the Java instance of the SDLActivity
	jobject activity = (jobject)SDL_GetAndroidActivity();

	// find the Java class of the activity. It should be SDLActivity or a subclass of it.
	jclass clazz(env->GetObjectClass(activity));

	// find the identifier of the method to call
	jmethodID method_id = env->GetStaticMethodID(clazz, "chooseFile", "(ILjava/lang/String;)V");

	jstring jstr = env->NewStringUTF(filename.c_str());

	// effectively call the Java method
	env->CallStaticVoidMethod(clazz, method_id, is_save, jstr);

	// clean up the local references.
	env->DeleteLocalRef(jstr);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

}


void OnChooseFile(int fd, int is_save, const std::string& filename);

#if 0
ims_static AAssetManager* g_asset_manger = nullptr;
#endif

extern "C" {
	JNIEXPORT void JNICALL 
		Java_com_ifstile_IFStile_nativeOnFDReady(
			JNIEnv* env, jclass,
			jint fd, jint requestCode, jstring uri)
	{
		jboolean isCopy;
		let* str = env->GetStringUTFChars(uri, &isCopy);
		std::string filename;
		if (str)filename = str;
		env->ReleaseStringUTFChars(uri, str);
		OnChooseFile(fd, (int)requestCode, filename);
	}
#if 0
	JNIEXPORT void JNICALL 
		Java_com_ifstile_IFStile_nativeSetAssetMgr(
			JNIEnv* env, jclass, jobject assetManager)
	{
		g_asset_manger = AAssetManager_fromJava(env, assetManager);
		assert(g_asset_manger);
	}
#endif
};

#if 0
static int android_read(void* cookie, char* buf, int size) {
	return AAsset_read((AAsset*)cookie, buf, size);
}

static int android_write(void* cookie, const char* buf, int size) {
	return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek(void* cookie, fpos_t offset, int whence) {
	return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_close(void* cookie) {
	AAsset_close((AAsset*)cookie);
	return 0;
}

FILE* android_fopen(const char* fname, const char* mode) {
	if (mode[0] == 'w') return nullptr;

	AAsset* asset = AAssetManager_open(g_asset_manger, fname, 0);
	if (!asset) return nullptr;

	return funopen(asset, android_read, android_write, android_seek, android_close);
}


//Usage:
//AAsset* asset = AAssetManager_open(mgr, "some_asset.bin", AASSET_MODE_BUFFER);
//asset_streambuf sb(asset);
//std::istream is(&sb);
struct asset_streambuf : public std::streambuf 
{
	asset_streambuf(AAsset* a): m_a(a) 
	{
		char* begin = (char*)AAsset_getBuffer(a);
		char* end = begin + AAsset_getLength64(a);
		setg(begin, begin, end);
	}
	~asset_streambuf() {
		AAsset_close(m_a);
	}
private:
	AAsset* m_a;
};


bool ims_read_asset(std::string* dst, const std::string& filename)
{
	auto* asset = 
		AAssetManager_open(g_asset_manger, 
			filename.c_str(), AASSET_MODE_BUFFER);
	if (!asset)return false;

	asset_streambuf sb(asset);
	std::istream fs(&sb);

	if (!dst)return true;

	using fi = std::istreambuf_iterator<char>;
	dst->assign(fi(fs), fi());
	return true;
}


static const char* AndroidGetExternalPath(bool read)
{
	let es = SDL_AndroidGetExternalStorageState();
	let* p = SDL_AndroidGetExternalStoragePath();

	if (read) {
		if (es & SDL_ANDROID_EXTERNAL_STORAGE_READ)return p;
	}
	else {
		if (es & SDL_ANDROID_EXTERNAL_STORAGE_WRITE)return p;
	}
	return nullptr;
}
#endif

#endif //__ANDROID__
