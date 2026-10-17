#include "../FCTAPI.h"
#include "Runtime.h"
#include "Android_Runtime.h"


namespace FCT{
    Android_Runtime* g_androidRuntime;
    void Android_Runtime::init(JNIEnv *env, jobject activity) {
        JavaVM* vm;
        env->GetJavaVM(&vm);
        m_vm = new jni::JvmRef<jni::kDefaultJvm>{vm};
        m_activity = new jni::GlobalObject<FCTMainActivityJavaClass>(
                std::move(jni::LocalObject<FCTMainActivityJavaClass>
                        {activity}));
        fout << "FCT Device Info:" << std::endl;
        fout << "Size: " << getDeviceWidth() << "x" << getDeviceHeight() << std::endl;

        m_uiCommon = new Android_UICommon();
        m_uiCommon->preinit();

        m_userThread = new std::thread([this]{
            jni::ThreadGuard threadGuard = m_vm->BuildThreadGuard();
            entry();
            cleanUp();
        });
        m_userThread->detach();
    }

    void Android_Runtime::term() {

    }

    void Android_Runtime::entry() {
        main();
    }

    void Android_Runtime::cleanUp() {
        m_uiCommon->cleanup();
        delete m_uiCommon;
    }

    int Android_Runtime::getDeviceHeight() {
        return m_activity->Call<"getDeviceHeight">();
    }

    int Android_Runtime::getDeviceWidth() {
        return  m_activity->Call<"getDeviceWidth">();
    }
    struct Android_RuntimeCommon : public RuntimeCommon {

    };
    void Runtime::init() {
        g_common = FCT_NEW(Android_RuntimeCommon);
    }

    void Runtime::term() {
        FCT_DELETE(g_common);
    }

    int Runtime::getDeviceWidth() {
        return g_androidRuntime->getDeviceWidth();
    }

    int Runtime::getDeviceHeight() {
        return g_androidRuntime->getDeviceHeight();
    }

    Window *Runtime::createWindow() {
        return FCT_NEW(Android_Window);
    }

    Context *Runtime::createContext() {
        return FCT_NEW(VK_Context, g_common->vkContextCommon);
    }

    ImageLoader *Runtime::createImageLoader() {
        return FCT_NEW(Stb_ImageLoader);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTMainActivity_setNativeWorkingDir(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    if (!path) {
        return;
    }
    const char *utfChars = env->GetStringUTFChars(path, nullptr);
    if (utfChars) {
        std::error_code ec;
        std::filesystem::current_path(std::filesystem::u8path(utfChars), ec);
        env->ReleaseStringUTFChars(path, utfChars);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_androidtutorial_FCTMainActivity_CPPEntry(JNIEnv *env, jobject thiz) {
    FCT::g_androidRuntime = new FCT::Android_Runtime();
    FCT::g_androidRuntime->init(env, thiz);
}
