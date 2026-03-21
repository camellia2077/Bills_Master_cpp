# Keep JNI entry points stable for the native bridge.
-keep class com.billstracer.android.data.BillsNativeBindings {
    native <methods>;
}
