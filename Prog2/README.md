```
export ANDROID_NDK=$HOME/Android/android-ndk-r27c
mkdir _build
pushd _build
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a ..
make
popd
```