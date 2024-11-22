export ANDROID_NDK=$HOME/android-ndk-r27b
mkdir _build
pushd _build
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a ..
make
popd
