/*
Copyright 2016 Silent Circle, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifdef ANDROID
#include <android/log.h>
#else
#include <stdio.h>
#endif

#include "axolotl_AxolotlNative.h"
#include "../AppInterfaceImpl.h"
#include "../../provisioning/Provisioning.h"
#include "../../appRepository/AppRepository.h"
#include "../../interfaceTransport/sip/SipTransport.h"
#include "../../axolotl/state/AxoConversation.h"
#include "../../axolotl/crypto/EcCurve.h"
#include "../../Constants.h"
#include "../../util/cJSON.h"
#include "../../attachments/fileHandler/scloud.h"
#include "../../storage/NameLookup.h"

using namespace axolotl;
using namespace std;

/**
 * Define -DPACKAGE_NAME="Java_some_package_name_" to define another package 
 * name during compilation
 */
#ifndef PACKAGE_NAME
#define PACKAGE_NAME Java_axolotl_AxolotlNative_
#endif

#define CONCATx(a,b) a##b
#define CONCAT(a,b) CONCATx(a,b)

#define JNI_FUNCTION(FUNC_NAME)  CONCAT(PACKAGE_NAME, FUNC_NAME)


#define LOGGING
#ifdef LOGGING
#define LOG(deb)   deb
#else
#define LOG(deb)
#endif

#ifdef EMBEDDED
JavaVM *t_getJavaVM();
#endif

static AppInterfaceImpl* axoAppInterface = NULL;
static JavaVM* javaVM = NULL;

// Set in doInit(...)
static jobject axolotlCallbackObject = NULL;
static jmethodID receiveMessageCallback = NULL;
static jmethodID storeMessageCallback = NULL;
static jmethodID stateReportCallback = NULL;
static jmethodID httpHelperCallback = NULL;
static jmethodID javaNotifyCallback = NULL;
static jmethodID groupMsgReceiveCallback = NULL;
static jmethodID groupCmdReceiveCallback = NULL;
static jmethodID groupStateCallback = NULL;

static int32_t debugLevel = 1;

// Plain public API without a class
AppInterface* j_getAxoAppInterface() { return axoAppInterface; }

void Log(char const *format, ...) {
    va_list arg;
    va_start(arg, format);
#ifdef ANDROID
    LOG(if (debugLevel > 0) __android_log_vprint(ANDROID_LOG_DEBUG, "axolotl", format, arg);)
#else
    LOG(if (debugLevel > 0){ vfprintf(stderr, format, arg); fprintf(stderr, "\n");})
#endif
    va_end(arg);
}

// typedef void (*SEND_DATA_FUNC)(uint8_t* [], uint8_t* [], uint8_t* [], size_t [], uint64_t []);
#ifdef UNITTESTS
// names, devIds, envelopes, sizes, msgIds
static bool sendDataFuncTesting(uint8_t* names, uint8_t* devIds, uint8_t* envelopes, size_t sizes, uint64_t msgIds)
{
    Log("sendData: %s - %s - %s\n", names, devIds, envelopes);

    string fName((const char*)names);
    fName.append((const char*)devIds).append(".msg");

    FILE* msgFile = fopen(fName.c_str(), "w");

    size_t num = fwrite(envelopes, 1, sizes, msgFile);
    Log("Message file written: %d bytes\n", num);
    fclose(msgFile);
    return true;
}

static void receiveData(const string &msgFileName)
{
    uint8_t msgData[2000];
    FILE* msgFile = fopen(msgFileName.c_str(), "r");
    if (msgFile == NULL) {
        Log("Message file %s not found\n", msgFileName.c_str());
        return;
    }

    size_t num = fread(msgData, 1, 2000, msgFile);
    Log("Message file read: %d bytes\n", num);
    axoAppInterface->getTransport()->receiveAxoMessage(msgData, num);
    fclose(msgFile);

}
#endif

static bool arrayToString(JNIEnv* env, jbyteArray array, string* output)
{
    if (array == NULL)
        return false;

    size_t dataLen = static_cast<size_t>(env->GetArrayLength(array));
    if (dataLen == 0)
        return false;

    const uint8_t* tmp = (uint8_t*)env->GetByteArrayElements(array, 0);
    if (tmp == NULL)
        return false;

    output->assign((const char*)tmp, dataLen);
    env->ReleaseByteArrayElements(array, (jbyte*)tmp, 0);
    return true;
}

static jbyteArray stringToArray(JNIEnv* env, const string& input)
{
    if (input.size() == 0)
        return NULL;

    jbyteArray data = env->NewByteArray(static_cast<jsize>(input.size()));
    if (data == NULL)
        return NULL;
    env->SetByteArrayRegion(data, 0, static_cast<jsize>(input.size()), (jbyte*)input.data());
    return data;
}

static void setReturnCode(JNIEnv* env, jintArray codeArray, int32_t result, int32_t data = 0)
{
    jint* code = env->GetIntArrayElements(codeArray, 0);
    code[0] = result;
    if (data != 0)
        code[1] = data;
    env->ReleaseIntArrayElements(codeArray, code, 0);
}


/**
 * Local helper class to keep track of thread attach / thread detach
 */
class CTJNIEnv {
    JNIEnv *env;
    bool attached;
public:
    CTJNIEnv() : attached(false), env(NULL) {

#ifdef EMBEDDED
        if (!javaVM)
            javaVM = t_getJavaVM();
#endif

        if (!javaVM)
            return;

        int s = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (s != JNI_OK){
#ifdef ANDROID
            s = javaVM->AttachCurrentThread(&env, NULL);
#else
            s = javaVM->AttachCurrentThread((void**)&env, NULL);
#endif
            if (!env || s < 0) {
                env = NULL;
                return;
            }
            attached = true;
        }
    }

    ~CTJNIEnv() {
        if (attached && javaVM)
            javaVM->DetachCurrentThread();
    }

    JNIEnv *getEnv() {
        return env;
    }
};

// A global symbol to force loading of the object in case of embedded usage
void loadAxolotl() 
{
}

/*
 * Receive message callback for AppInterfaceImpl.
 * 
 * "([B[B[B)I"
 */
static int32_t receiveMessage(const string& messageDescriptor, const string& attachmentDescriptor = string(), const string& messageAttributes = string())
{
    if (axolotlCallbackObject == NULL)
        return -1;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return -2;

    jbyteArray message = stringToArray(env, messageDescriptor);
    Log("receiveMessage - message: '%s' - length: %d", messageDescriptor.c_str(), messageDescriptor.size());

    jbyteArray attachment = NULL;
    if (!attachmentDescriptor.empty()) {
        attachment = stringToArray(env, attachmentDescriptor);
        if (attachment == NULL) {
            return -4;
        }
    }
    jbyteArray attributes = NULL;
    if (!messageAttributes.empty()) {
        attributes = stringToArray(env, messageAttributes);
        if (attributes == NULL) {
            return -4;
        }
    }
    int32_t result = env->CallIntMethod(axolotlCallbackObject, receiveMessageCallback, message, attachment, attributes);

    env->DeleteLocalRef(message);
    if (attachment != NULL)
        env->DeleteLocalRef(attachment);
    if (attributes != NULL)
        env->DeleteLocalRef(attributes);

    return result;
}

/*
 * Store message data callback for AppInterfaceImpl.
 *
 * "([B[B[B)I"
 */
static int32_t storeMessageData(const string& messageDescriptor, const string& attachmentDescriptor = string(), const string& messageAttributes = string())
{
    if (axolotlCallbackObject == NULL)
        return -1;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return -2;

    jbyteArray message = stringToArray(env, messageDescriptor);
    Log("storeMessageData - message: '%s' - length: %d", messageDescriptor.c_str(), messageDescriptor.size());

    jbyteArray attachment = NULL;
    if (!attachmentDescriptor.empty()) {
        attachment = stringToArray(env, attachmentDescriptor);
        if (attachment == NULL) {
            return -4;
        }
    }
    jbyteArray attributes = NULL;
    if (!messageAttributes.empty()) {
        attributes = stringToArray(env, messageAttributes);
        if (attributes == NULL) {
            return -4;
        }
    }
    int32_t result = env->CallIntMethod(axolotlCallbackObject, storeMessageCallback, message, attachment, attributes);

    env->DeleteLocalRef(message);
    if (attachment != NULL)
        env->DeleteLocalRef(attachment);
    if (attributes != NULL)
        env->DeleteLocalRef(attributes);

    return result;
}

/*
 * Receive message callback for AppInterfaceImpl.
 *
 * "([B[B[B)I"
 */
static int32_t receiveGroupMessage(const string& messageDescriptor, const string& attachmentDescriptor = string(), const string& messageAttributes = string())
{
    if (axolotlCallbackObject == NULL)
        return -1;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return -2;

    jbyteArray message = stringToArray(env, messageDescriptor);
    Log("receiveGroupMessage: '%s' - length: %d", messageDescriptor.c_str(), messageDescriptor.size());

    jbyteArray attachment = NULL;
    if (!attachmentDescriptor.empty()) {
        attachment = stringToArray(env, attachmentDescriptor);
        if (attachment == NULL) {
            return -4;
        }
    }
    jbyteArray attributes = NULL;
    if (!messageAttributes.empty()) {
        attributes = stringToArray(env, messageAttributes);
        if (attributes == NULL) {
            return -4;
        }
    }
    int32_t result = env->CallIntMethod(axolotlCallbackObject, groupMsgReceiveCallback, message, attachment, attributes);

    env->DeleteLocalRef(message);
    if (attachment != NULL)
        env->DeleteLocalRef(attachment);
    if (attributes != NULL)
        env->DeleteLocalRef(attributes);

    return result;
}

/*
 * Receive message callback for AppInterfaceImpl.
 *
 * "([B[B[B)I"
 */
static int32_t receiveGroupCommand(const string& commandMessage)
{
    if (axolotlCallbackObject == NULL)
        return -1;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return -2;

    jbyteArray message = stringToArray(env, commandMessage);
    Log("receiveGroupCommand: '%s' - length: %d", commandMessage.c_str(), commandMessage.size());

    int32_t result = env->CallIntMethod(axolotlCallbackObject, groupCmdReceiveCallback, message);

    env->DeleteLocalRef(message);

    return result;
}

/*
 * State change callback for AppInterfaceImpl.
 * 
 * "(J[B)V"
 */
static void messageStateReport(int64_t messageIdentifier, int32_t statusCode, const string& stateInformation)
{
    if (axolotlCallbackObject == NULL)
        return;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return;

    jbyteArray information = NULL;
    if (!stateInformation.empty()) {
        information = stringToArray(env, stateInformation);
    }
    env->CallVoidMethod(axolotlCallbackObject, stateReportCallback, messageIdentifier, statusCode, information);
    if (information != NULL)
        env->DeleteLocalRef(information);
}

/*
 * State change callback for AppInterfaceImpl.
 *
 * "(J[B)V"
 */
static void groupStateReport(int32_t statusCode, const string& stateInformation)
{
    if (axolotlCallbackObject == NULL)
        return;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return;

    jbyteArray information = NULL;
    if (!stateInformation.empty()) {
        information = stringToArray(env, stateInformation);
    }
    env->CallVoidMethod(axolotlCallbackObject, groupStateCallback, statusCode, information);
    if (information != NULL)
        env->DeleteLocalRef(information);
}

/*
 * Notify callback for AppInterfaceImpl.
 * 
 * "(I[B[B)V"
 */
static void notifyCallback(int32_t notifyAction, const string& actionInformation, const string& devId)
{
    if (axolotlCallbackObject == NULL)
        return;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();
    if (!env)
        return;

    jbyteArray information = NULL;
    if (!actionInformation.empty()) {
        information = stringToArray(env, actionInformation);
    }
    jbyteArray deviceId = NULL;
    if (!devId.empty()) {
        deviceId = stringToArray(env, devId);
    }
    env->CallVoidMethod(axolotlCallbackObject, javaNotifyCallback, notifyAction, information, deviceId);

    if (information != NULL)
        env->DeleteLocalRef(information);

    if (deviceId != NULL)
        env->DeleteLocalRef(deviceId);
}

/*
 * Class:     AxolotlNative
 * Method:    httpHelper
 * Signature: ([BLjava/lang/String;[B[I)[B
 */
/*
 * HTTP request helper callback for provisioning etc.
 */
#define JAVA_HELPER
#if defined JAVA_HELPER || defined UNITTESTS
static int32_t httpHelper(const string& requestUri, const string& method, const string& requestData, string* response)
{
    if (axolotlCallbackObject == NULL)
        return -1;

    CTJNIEnv jni;
    JNIEnv *env = jni.getEnv();

    if (!env) {
        return -2;
    }

    jbyteArray uri = NULL;
    uri = env->NewByteArray(static_cast<jsize>(requestUri.size()));
    if (uri == NULL)
        return -3;
    env->SetByteArrayRegion(uri, 0, static_cast<jsize>(requestUri.size()), (jbyte*)requestUri.data());

    jbyteArray reqData = NULL;
    if (!requestData.empty()) {
        reqData = stringToArray(env, requestData);
    }
    jstring mthod = env->NewStringUTF(method.c_str());

    jintArray code = env->NewIntArray(1);

    jbyteArray data = (jbyteArray)env->CallObjectMethod(axolotlCallbackObject, httpHelperCallback, uri, mthod, reqData, code);
     if (data != NULL) {
        arrayToString(env, data, response);
    }
    int32_t result = -1;
    env->GetIntArrayRegion(code, 0, 1, &result);

    env->DeleteLocalRef(uri);
    if (reqData != NULL)
        env->DeleteLocalRef(reqData);
    env->DeleteLocalRef(mthod);
    env->DeleteLocalRef(code);

    return result;
}
#else
static int32_t httpHelper(const string& requestUri, const string& method, const string& requestData, string* response)
{

    char* t_send_http_json(const char *url, const char *meth,  char *bufResp, int iMaxLen, int &iRespContentLen, const char *pContent);

    Log("httpHelper request, method: '%s', url '%s'", method.c_str(), requestUri.c_str());
    if (requestData.size() > 0) {
        Log("httpHelper request, data: '%s'", requestData.c_str());
    }

    int iSizeOfRet = 4 * 1024;
    char *retBuf = new char [iSizeOfRet];
    int iContentLen = 0;

    int code = 0;
    char *content = t_send_http_json (requestUri.c_str(), method.c_str(), retBuf, iSizeOfRet - 1, iContentLen, requestData.c_str());

    Log("httpHelper response data: '%s'", content ? content : "No response data");

    if(content && iContentLen > 0 && response)
        response->assign((const char*)content, iContentLen);

    delete retBuf;

    if(iContentLen < 1)
        return -1;
   return 200;
}
#endif

#ifndef EMBEDDED
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    (void)reserved;
    javaVM = vm;
    return JNI_VERSION_1_6;
}
#endif

/*
 * Class:     axolotl_AxolotlNative
 * Method:    doInit
 * Signature: (ILjava/lang/String;[B[B[B[BZ)I
 */
JNIEXPORT jint JNICALL 
JNI_FUNCTION(doInit)(JNIEnv* env, jobject thiz, jint flags, jstring dbName, jbyteArray dbPassphrase, jbyteArray userName,
                    jbyteArray authorization, jbyteArray scClientDeviceId, jboolean delayRatchetCommit)
{
    debugLevel = flags & 0xf;
//    int32_t flagsInternal = flags >> 4;

    if (axolotlCallbackObject == NULL) {
        axolotlCallbackObject = env->NewGlobalRef(thiz);
        if (axolotlCallbackObject == NULL) {
            return -1;
        }
        jclass callbackClass = NULL;
        callbackClass = env->GetObjectClass(axolotlCallbackObject);
        if (callbackClass == NULL) {
            return -2;
        }
        receiveMessageCallback = env->GetMethodID(callbackClass, "receiveMessage", "([B[B[B)I");
        if (receiveMessageCallback == NULL) {
            return -3;
        }
        stateReportCallback = env->GetMethodID(callbackClass, "messageStateReport", "(JI[B)V");
        if (stateReportCallback == NULL) {
            return -4;
        }
        httpHelperCallback = env->GetMethodID(callbackClass, "httpHelper", "([BLjava/lang/String;[B[I)[B");
        if (httpHelperCallback == NULL) {
            return -5;
        }
        javaNotifyCallback = env->GetMethodID(callbackClass, "notifyCallback", "(I[B[B)V");
        if (javaNotifyCallback == NULL) {
            return -6;
        }
        groupMsgReceiveCallback = env->GetMethodID(callbackClass, "groupMsgReceive", "([B[B[B)I");
        if (groupMsgReceiveCallback == NULL) {
            return -20;
        }
        groupCmdReceiveCallback = env->GetMethodID(callbackClass, "groupCmdReceive", "([B)I");
        if (groupCmdReceiveCallback == NULL) {
            return -21;
        }
        groupStateCallback = env->GetMethodID(callbackClass, "groupStateCallback", "(I[B)V");
        if (groupStateCallback == NULL) {
            return -22;
        }
        storeMessageCallback = env->GetMethodID(callbackClass, "storeMessageData", "([B[B[B)I");
        if (storeMessageCallback == NULL) {
            return -23;
        }
    }
    string name;
    if (!arrayToString(env, userName, &name)) {
        return -10;
    }

    string auth;
    if (!arrayToString(env, authorization, &auth)) {
        return -11;
    }

    string devId;
    if (!arrayToString(env, scClientDeviceId, &devId))
        return -12;

    const uint8_t* pw = (uint8_t*)env->GetByteArrayElements(dbPassphrase, 0);
    size_t pwLen = static_cast<size_t>(env->GetArrayLength(dbPassphrase));
    if (pw == NULL)
        return -14;

    if (pwLen != 32) {
        env->ReleaseByteArrayElements(dbPassphrase, (jbyte*)pw, 0);
        return -15;
    }

    if (dbName == NULL)
        return -16;

    string dbPw((const char*)pw, pwLen);

    memset_volatile((void*)pw, 0, pwLen);
    env->ReleaseByteArrayElements(dbPassphrase, (jbyte*)pw, 0);

    // initialize and open the persitent store singleton instance
    SQLiteStoreConv* store = SQLiteStoreConv::getStore();
    store->setKey(dbPw);

    const char* db = env->GetStringUTFChars(dbName, 0);
    store->openStore(string (db));
    env->ReleaseStringUTFChars(dbName, db);

    memset_volatile((void*)dbPw.data(), 0, dbPw.size());

    int32_t retVal = 1;
    AxoConversation* ownAxoConv = AxoConversation::loadLocalConversation(name);
    if (!ownAxoConv->isValid()) {  // no yet available, create one. An own conversation has the same local and remote name, empty device id
        const DhKeyPair* idKeyPair = EcCurve::generateKeyPair(EcCurveTypes::Curve25519);
        ownAxoConv->setDHIs(idKeyPair);
        ownAxoConv->storeConversation();
        retVal = 2;
    }
    delete ownAxoConv;    // Not needed anymore here

    axoAppInterface = new AppInterfaceImpl(name, auth, devId, receiveMessage, storeMessageData, messageStateReport,
                                           notifyCallback, receiveGroupMessage, receiveGroupCommand, groupStateReport);
    axoAppInterface->setDelayRatchetCommit(delayRatchetCommit != 0);
    Transport* sipTransport = new SipTransport(axoAppInterface);

    /* ***********************************************************************************
     * Initialize pointers/callback to the send/receive SIP data functions (network layer) 
     */
#ifdef UNITTESTS
    sipTransport->setSendDataFunction(sendDataFuncTesting);
#elif defined (EMBEDDED)
    // Functions defined in t_a_main module of silentphone library, this sends the data
    // via SIP message
//    void g_sendDataFuncAxo(uint8_t* names[], uint8_t* devIds[], uint8_t* envelopes[], size_t sizes[], uint64_t msgIds[]);
    bool g_sendDataFuncAxoNew(uint8_t* names, uint8_t* devId, uint8_t* envelope, size_t size, uint64_t msgIds);
    void t_setAxoTransport(Transport *transport);

    sipTransport->setSendDataFunction(g_sendDataFuncAxoNew);
    t_setAxoTransport(sipTransport);
#else
#error "***** Missing initialization."
#endif
    /* *********************************************************************************
     * set sipTransport class to SIP network handler, sipTransport contains callback
     * functions 'receiveAxoData' and 'stateReportAxo'
     *********************************************************************************** */
    axoAppInterface->setHttpHelper(httpHelper);
    axoAppInterface->setTransport(sipTransport);

    return retVal;
}

/*
 * Class:     AxolotlNative
 * Method:    sendMessage
 * Signature: ([B[B[B)[J
 */
JNIEXPORT jlongArray JNICALL 
JNI_FUNCTION(sendMessage)(JNIEnv* env, jclass clazz, jbyteArray messageDescriptor, jbyteArray attachementDescriptor, jbyteArray messageAttributes)
{
    (void)clazz;

    if (messageDescriptor == NULL || axoAppInterface == NULL)
        return 0L;

    string message;
    if (!arrayToString(env, messageDescriptor, &message)) {
        return 0L;
    }
    Log("sendMessage - message: '%s' - length: %d", message.c_str(), message.size());

    string attachment;
    if (attachementDescriptor != NULL) {
        arrayToString(env, attachementDescriptor, &attachment);
        Log("sendMessage - attachement: '%s' - length: %d", attachment.c_str(), attachment.size());
    }
    string attributes;
    if (messageAttributes != NULL) {
        arrayToString(env, messageAttributes, &attributes);
        Log("sendMessage - attributes: '%s' - length: %d", attributes.c_str(), attributes.size());
    }
    vector<int64_t>* msgIds = axoAppInterface->sendMessage(message, attachment, attributes);
    if (msgIds == NULL || msgIds->empty()) {
        delete msgIds;
        return NULL;
    }
    size_t size = msgIds->size();

    jlongArray result = NULL;
    result = env->NewLongArray(static_cast<jsize>(size));
    jlong* resultArray = env->GetLongArrayElements(result, 0);

    for(size_t i = 0; i < size; i++) {
        resultArray[i] = static_cast<jlong>(msgIds->at(i));  // long -> jlong -> signed 64 bits, primitive type mapping
    }
    env->ReleaseLongArrayElements(result, resultArray, 0);
    delete msgIds;
    return result;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    sendMessageToSiblings
 * Signature: ([B[B[B)[J
 */
JNIEXPORT jlongArray JNICALL
JNI_FUNCTION(sendMessageToSiblings) (JNIEnv* env, jclass clazz, jbyteArray messageDescriptor, jbyteArray attachementDescriptor, jbyteArray messageAttributes)
{
    (void)clazz;

    if (messageDescriptor == NULL || axoAppInterface == NULL)
        return 0L;

    string message;
    if (!arrayToString(env, messageDescriptor, &message)) {
        return 0L;
    }
    Log("sendMessage sib - message: '%s' - length: %d", message.c_str(), message.size());

    string attachment;
    if (attachementDescriptor != NULL) {
        arrayToString(env, attachementDescriptor, &attachment);
        Log("sendMessage sib - attachement: '%s' - length: %d", attachment.c_str(), attachment.size());
    }
    string attributes;
    if (messageAttributes != NULL) {
        arrayToString(env, messageAttributes, &attributes);
        Log("sendMessage sib - attributes: '%s' - length: %d", attributes.c_str(), attributes.size());
    }
    vector<int64_t>* msgIds = axoAppInterface->sendMessageToSiblings(message, attachment, attributes);
    if (msgIds == NULL || msgIds->empty()) {
        delete msgIds;
        return NULL;
    }
    size_t size = msgIds->size();

    jlongArray result = NULL;
    result = env->NewLongArray(static_cast<jsize>(size));
    jlong* resultArray = env->GetLongArrayElements(result, 0);

    for(size_t i = 0; i < size; i++) {
        resultArray[i] = static_cast<jlong>(msgIds->at(i));
    }
    env->ReleaseLongArrayElements(result, resultArray, 0);
    delete msgIds;
    return result;
}


/*
 * Class:     AxolotlNative
 * Method:    getKnownUsers
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL 
JNI_FUNCTION(getKnownUsers)(JNIEnv* env, jclass clazz)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    string* jsonNames = axoAppInterface->getKnownUsers();
    if (jsonNames == NULL)
        return NULL;

    size_t size = jsonNames->size();
    jbyteArray names = NULL;
    names = env->NewByteArray(static_cast<jsize>(size));
    if (names != NULL) {
        env->SetByteArrayRegion(names, 0, static_cast<jsize>(size), (jbyte*)jsonNames->data());
    }
    delete jsonNames;
    return names;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getOwnIdentityKey
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getOwnIdentityKey) (JNIEnv* env, jclass clazz)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    string idKey = axoAppInterface->getOwnIdentityKey();
    jbyteArray key = stringToArray(env, idKey);
    return key;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getIdentityKeys
 * Signature: ([B)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(getIdentityKeys) (JNIEnv* env, jclass clazz, jbyteArray userName)
{
    (void)clazz;

    string name;
    if (!arrayToString(env, userName, &name) || axoAppInterface == NULL)
        return NULL;

    list<string>* idKeys = axoAppInterface->getIdentityKeys(name);

    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(idKeys->size()), byteArrayClass, NULL);

    int32_t index = 0;
    while (!idKeys->empty()) {
        string s = idKeys->front();
        idKeys->pop_front();
        jbyteArray retData = stringToArray(env, s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    delete idKeys;
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getAxoDevicesUser
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getAxoDevicesUser) (JNIEnv* env, jclass clazz, jbyteArray userName)
{
    (void)clazz;

    string name;
    if (!arrayToString(env, userName, &name) || axoAppInterface == NULL)
        return NULL;

    shared_ptr<list<pair<string, string> > > devices = Provisioning::getAxoDeviceIds(name, axoAppInterface->getOwnAuthrization());

    if (!devices || devices->empty()) {
        return NULL;
    }

    cJSON *root,*devArray, *devInfo;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "version", cJSON_CreateNumber(1));
    cJSON_AddItemToObject(root, "devices", devArray = cJSON_CreateArray());

    while (!devices->empty()) {
        pair<string, string> idName = devices->front();
        devices->pop_front();
        devInfo = cJSON_CreateObject();
        cJSON_AddStringToObject(devInfo, "id", idName.first.c_str());
        cJSON_AddStringToObject(devInfo, "device_name", idName.second.c_str());
        cJSON_AddItemToArray(devArray, devInfo);
    }

    char *out = cJSON_Print(root);
    string json(out);
    cJSON_Delete(root); free(out);

    jbyteArray retData = stringToArray(env, json);
    return retData;
}


/*
 * Class:     AxolotlNative
 * Method:    registerAxolotlDevice
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL 
JNI_FUNCTION(registerAxolotlDevice)(JNIEnv* env, jclass clazz, jintArray code)
{
    (void)clazz;

    string info;
    if (code == NULL || env->GetArrayLength(code) < 1 || axoAppInterface == NULL)
        return NULL;

    int32_t result = axoAppInterface->registerAxolotlDevice(&info);

    setReturnCode(env, code, result);

    jbyteArray infoBytes = NULL;
    if (!info.empty()) {
        size_t size = info.size();
        infoBytes = env->NewByteArray(static_cast<jsize>(size));
        if (infoBytes != NULL) {
            env->SetByteArrayRegion(infoBytes, 0, static_cast<jsize>(size), (jbyte*)info.data());
        }
    }
    return infoBytes;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    removeAxolotlDevice
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(removeAxolotlDevice) (JNIEnv* env, jclass clazz, jbyteArray deviceId, jintArray code)
{
    (void)clazz;

    string info;
    if (code == NULL || env->GetArrayLength(code) < 1 || axoAppInterface == NULL)
        return NULL;

    string devId;
    if (!arrayToString(env, deviceId, &devId))
        return NULL;


    int32_t result = axoAppInterface->removeAxolotlDevice(devId, &info);

    setReturnCode(env, code, result);

    jbyteArray infoBytes = NULL;
    if (!info.empty()) {
        size_t size = info.size();
        infoBytes = env->NewByteArray(static_cast<jsize>(size));
        if (infoBytes != NULL) {
            env->SetByteArrayRegion(infoBytes, 0, static_cast<jsize>(size), (jbyte*)info.data());
        }
    }
    return infoBytes;
}


/*
 * Class:     axolotl_AxolotlNative
 * Method:    newPreKeys
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL 
JNI_FUNCTION(newPreKeys)(JNIEnv* env, jclass clazz, jint numbers)
{
    (void)clazz;
    (void)env;
    if (axoAppInterface == NULL)
        return -1;

    return axoAppInterface->newPreKeys(numbers);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getNumPreKeys
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(getNumPreKeys) (JNIEnv* env, jclass clazz)
{
    (void)clazz;
    (void)env;
    if (axoAppInterface == NULL)
        return -1;

    return axoAppInterface->getNumPreKeys();
}

/*
 * Class:     AxolotlNative
 * Method:    getErrorCode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(getErrorCode)(JNIEnv* env, jclass clazz)
{
    (void)clazz;
    (void)env;
    if (axoAppInterface == NULL)
        return -1;

    return axoAppInterface->getErrorCode();
}

/*
 * Class:     AxolotlNative
 * Method:    getErrorInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JNI_FUNCTION(getErrorInfo)(JNIEnv* env, jclass clazz)
{
    (void)clazz;
    if (axoAppInterface == NULL)
        return NULL;

    const string info = axoAppInterface->getErrorInfo();
    jstring errInfo = env->NewStringUTF(info.c_str());
    return errInfo;
}

/*
 * Class:     AxolotlNative
 * Method:    testCommand
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(testCommand)(JNIEnv* env, jclass clazz, jstring command, jbyteArray data)
{
    (void)clazz;

    int32_t result = 0;
    const char* cmd = env->GetStringUTFChars(command, 0);

    string dataContainer;
    if (data != NULL) {
        size_t dataLen = static_cast<size_t>(env->GetArrayLength(data));
        if (dataLen > 0) {
            const uint8_t* tmp = (uint8_t*)env->GetByteArrayElements(data, 0);
            if (tmp != NULL) {
                dataContainer.assign((const char*)tmp, dataLen);
                env->ReleaseByteArrayElements(data, (jbyte*)tmp, 0);
            }
        }
    }
    Log("testCommand - command: '%s' - data: '%s'", cmd, dataContainer.c_str());

#ifdef UNITTESTS
    if (strcmp("http", cmd) == 0) {
        string resultData;
        result = httpHelper(string("/some/request"), dataContainer, string("MTH"), &resultData);
        Log("httpHelper - code: %d, resultData: %s", result, resultData.c_str());
    }

    if (strcmp("read", cmd) == 0) {
        receiveData(dataContainer);
    }
#endif
    if (strcmp("resetaxodb", cmd) == 0) {
        SQLiteStoreConv* store = SQLiteStoreConv::getStore();
        store->resetStore();
        Log("Resetted Axolotl store");
    }

    env->ReleaseStringUTFChars(command, cmd);
    return result;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    axoCommand
 * Signature: (Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JNI_FUNCTION(axoCommand) (JNIEnv* env, jclass clazz, jstring command, jbyteArray data)
{
    (void)clazz;

    if (command == NULL || axoAppInterface == NULL)
        return NULL;
    const char* cmd = env->GetStringUTFChars(command, 0);

    jstring result = NULL;

    string dataContainer;
    arrayToString(env, data, &dataContainer);

    if (strcmp("removeAxoConversation", cmd) == 0 && !dataContainer.empty()) {
        Log("Removing Axolotl conversation data for '%s'\n", dataContainer.c_str());

        SQLiteStoreConv* store = SQLiteStoreConv::getStore();
        int32_t sqlResult = 0;
        store->deleteConversationsName(dataContainer, axoAppInterface->getOwnUser(), &sqlResult);

        Log("Removing Axolotl conversation data for '%s' returned %d\n", dataContainer.c_str(), sqlResult);
        if (SQL_FAIL(sqlResult)) {
            result = env->NewStringUTF(store->getLastError());
        }
        else {
            axoAppInterface->rescanUserDevices(dataContainer);
        }
    }
    else if (strcmp("rescanUserDevices", cmd) == 0 && !dataContainer.empty()) {
        axoAppInterface->rescanUserDevices(dataContainer);
    }
    else if (strcmp("reSyncConversation", cmd) == 0 && !dataContainer.empty()) {
        cJSON* root = cJSON_Parse(dataContainer.c_str());
        cJSON* details = NULL;
        if (root != NULL) {
            details = cJSON_GetObjectItem(root, "details");
        }
        if (details != NULL) {

            string userName;
            cJSON *jsonItem = cJSON_GetObjectItem(details, "name");
            if (jsonItem != NULL) {
                userName.assign(jsonItem->valuestring);
            }
            string deviceId;
            jsonItem = cJSON_GetObjectItem(details, "scClientDevId");
            if (jsonItem != NULL) {
                deviceId.assign(jsonItem->valuestring);
            }
            if (!userName.empty() && !deviceId.empty()) {
                axoAppInterface->reSyncConversation(userName, deviceId);
            }
        }
        cJSON_Delete(root);
    } else if (strcmp("clearGroupData", cmd) == 0) {
        axoAppInterface->clearGroupData();
    }
    env->ReleaseStringUTFChars(command, cmd);
    return result;
}

/*
 * **************************************************************
 * Below the native functions for group chat
 * *************************************************************
 */

/*
 * Class:     axolotl_AxolotlNative
 * Method:    createNewGroup
 * Signature: ([B[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JNI_FUNCTION(createNewGroup)(JNIEnv *env, jclass clazz, jbyteArray groupName, jbyteArray groupDescription, jint maxMembers)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    string group;
    if (!arrayToString(env, groupName, &group)) {
        return NULL;
    }
    string description;
    if (!arrayToString(env, groupDescription, &description)) {
        return NULL;
    }
    string groupUuid = axoAppInterface->createNewGroup(group, description, maxMembers);
    if (groupUuid.empty())
        return NULL;
    jstring uuidJava = env->NewStringUTF(groupUuid.c_str());
    return uuidJava;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    modifyGroupSize
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean
JNICALL JNI_FUNCTION(modifyGroupSize)(JNIEnv *env, jclass clazz, jstring groupUuid, jint newSize)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return JNI_FALSE;

    if (groupUuid == NULL)
        return JNI_FALSE;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);
    bool result = axoAppInterface->modifyGroupSize(group, newSize);
    return result ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    listAllGroups
 * Signature: ([I)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(listAllGroups)(JNIEnv *env, jclass clazz, jintArray code)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    int32_t result;
    shared_ptr<list<shared_ptr<cJSON> > > groups = axoAppInterface->getStore()->listAllGroups( &result);
    setReturnCode(env, code, result);

    size_t size = groups->size();
    if (size == 0)
        return NULL;

    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(size), byteArrayClass, NULL);

    int32_t index = 0;
    for (auto it = groups->begin(); it != groups->end(); ++it) {
        char *out = cJSON_PrintUnformatted(it->get());
        jbyteArray retData = stringToArray(env, out);
        free(out);

        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    return retArray;

}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getGroup
 * Signature: (Ljava/lang/String;[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getGroup)(JNIEnv *env, jclass clazz, jstring groupUuid, jintArray code)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    if (groupUuid == NULL)
        return NULL;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);

    int32_t result;
    shared_ptr<cJSON> groupJson = axoAppInterface->getStore()->listGroup(group, &result);

    setReturnCode(env, code, result);
    char *out = cJSON_PrintUnformatted(groupJson.get());
    jbyteArray retArray = NULL;
    if (out != NULL) {
        retArray = stringToArray(env, out);
        free(out);
    }
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getAllGroupMembers
 * Signature: (Ljava/lang/String;[I)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(getAllGroupMembers)(JNIEnv *env, jclass clazz, jstring groupUuid, jintArray code)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    if (groupUuid == NULL)
        return NULL;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);

    int32_t result;
    shared_ptr<list<shared_ptr<cJSON> > > members = axoAppInterface->getStore()->getAllGroupMembers(group, &result);
    setReturnCode(env, code, result);

    size_t size = members->size();
    if (size == 0)
        return NULL;

    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(size), byteArrayClass, NULL);

    int32_t index = 0;
    for (auto it = members->begin(); it != members->end(); ++it) {
        char *out = cJSON_PrintUnformatted(it->get());
        jbyteArray retData = stringToArray(env, out);
        free(out);

        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getGroupMember
 * Signature: (Ljava/lang/String;[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getGroupMember) (JNIEnv *env, jclass clazz, jstring groupUuid, jbyteArray memberUuid, jintArray code)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    if (groupUuid == NULL)
        return NULL;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);

    string memberId;
    if (!arrayToString(env, memberUuid, &memberId)) {
        return NULL;
    }

    int32_t result;
    shared_ptr<cJSON> memberJson = axoAppInterface->getStore()->getGroupMember(group, memberId, &result);
    setReturnCode(env, code, result);

    char *out = cJSON_PrintUnformatted(memberJson.get());
    jbyteArray retArray = stringToArray(env, out);
    free(out);
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    inviteUser
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(inviteUser)(JNIEnv *env, jclass clazz, jstring groupUuid, jbyteArray userId)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return GENERIC_ERROR;

    if (groupUuid == NULL)
        return DATA_MISSING;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);

    string usr;
    if (!arrayToString(env, userId, &usr)) {
        return GROUP_MSG_DATA_INCONSISTENT;
    }
    return axoAppInterface->inviteUser(group, usr);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    answerInvitation
 * Signature: ([BZ[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(answerInvitation)(JNIEnv *env, jclass clazz, jbyteArray command, jboolean accept, jbyteArray reason)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return GENERIC_ERROR;

    string cmd;
    if (!arrayToString(env, command, &cmd)) {
        return GROUP_MSG_DATA_INCONSISTENT;
    }
    string rsn;
    if (reason != NULL) {
        arrayToString(env, reason, &rsn);
    }
    return axoAppInterface->answerInvitation(cmd, accept != 0, rsn);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    sendGroupMessage
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(sendGroupMessage)(JNIEnv *env, jclass clazz, jbyteArray messageDescriptor, jbyteArray attachmentDescriptor, jbyteArray messageAttributes)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return GENERIC_ERROR;

    string message;
    if (!arrayToString(env, messageDescriptor, &message)) {
        return GROUP_MSG_DATA_INCONSISTENT;
    }
    Log("sendGroupMessage - message: '%s' - length: %d", message.c_str(), message.size());

    string attachment;
    if (attachmentDescriptor != NULL) {
        arrayToString(env, attachmentDescriptor, &attachment);
        Log("sendGroupMessage - attachment: '%s' - length: %d", attachment.c_str(), attachment.size());
    }
    string attributes;
    if (messageAttributes != NULL) {
        arrayToString(env, messageAttributes, &attributes);
        Log("sendGroupMessage - attributes: '%s' - length: %d", attributes.c_str(), attributes.size());
    }
    int32_t result  = axoAppInterface->sendGroupMessage(message, attachment, attributes);
    return result;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    leaveGroup
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(leaveGroup)(JNIEnv *env, jclass clazz, jstring groupUuid)
{
    (void)clazz;

    if (axoAppInterface == NULL)
        return GENERIC_ERROR;

    if (groupUuid == NULL)
        return DATA_MISSING;

    string group;
    const char* temp = env->GetStringUTFChars(groupUuid, 0);
    group = temp;
    env->ReleaseStringUTFChars(groupUuid, temp);
    return axoAppInterface->leaveGroup(group);
}



/*
 * **************************************************************
 * Below the native functions for the repository database
 * *************************************************************
 */

static AppRepository* appRepository = NULL;


/*
 * Class:     axolotl_AxolotlNative
 * Method:    repoOpenDatabase
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(repoOpenDatabase) (JNIEnv* env, jclass clazz, jstring dbName, jbyteArray keyData)
{
    (void)clazz;

    string nameString;
    if (dbName != NULL) {
        const char* name = env->GetStringUTFChars(dbName, 0);
        nameString = name;
        env->ReleaseStringUTFChars(dbName, name);
    }
    const uint8_t* pw = (uint8_t*)env->GetByteArrayElements(keyData, 0);
    size_t pwLen = static_cast<size_t>(env->GetArrayLength(keyData));
    if (pw == NULL)
        return -2;
    if (pwLen != 32)
        return -3;

    string dbPw((const char*)pw, pwLen);

    memset_volatile((void*)pw, 0, pwLen);
    env->ReleaseByteArrayElements(keyData, (jbyte*)pw, 0);

    appRepository = AppRepository::getStore();
    appRepository->setKey(dbPw);
    appRepository->openStore(nameString);

    memset_volatile((void*)dbPw.data(), 0, dbPw.size());

    return appRepository->getSqlCode();
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    repoCloseDatabase
 * Signature: ()V
 */
JNIEXPORT void JNICALL
JNI_FUNCTION(repoCloseDatabase) (JNIEnv* env, jclass clazz) {
    (void)clazz;
    (void)env;

    if (appRepository != NULL)
        AppRepository::closeStore();
    appRepository = NULL;
}

#define IS_APP_REPO_OPEN    (appRepository != NULL && appRepository->isReady())
/*
 * Class:     axolotl_AxolotlNative
 * Method:    repoIsOpen
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
JNI_FUNCTION(repoIsOpen) (JNIEnv* env, jclass clazz)
{
    (void)clazz;
    (void)env;

    return static_cast<jboolean>(IS_APP_REPO_OPEN);
}


/*
 * Class:     axolotl_AxolotlNative
 * Method:    existConversation
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL
JNI_FUNCTION(existConversation) (JNIEnv* env, jclass clazz, jbyteArray namePattern)
{
    (void)clazz;

    string name;
    if (!arrayToString(env, namePattern, &name))
        return static_cast<jboolean>(false);

    if (!IS_APP_REPO_OPEN)
        return static_cast<jboolean>(false);

    bool result = appRepository->existConversation(name);
    return static_cast<jboolean>(result);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    storeConversation
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(storeConversation) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray convData)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    string name;
    if (!arrayToString(env, inName, &name))
        return -1;

    string data;
    arrayToString(env, convData, &data);
    return appRepository->storeConversation(name, data);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadConversation
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(loadConversation) (JNIEnv* env, jclass clazz, jbyteArray inName, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    string name;
    if (!arrayToString(env, inName, &name)) {
        setReturnCode(env, code, -1);
        return NULL;
    }

    string data;
    int32_t result = appRepository->loadConversation(name, &data);
    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        return NULL;
    }

    setReturnCode(env, code, result);
    jbyteArray retData = stringToArray(env, data);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    deleteConversation
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL 
JNI_FUNCTION(deleteConversation) (JNIEnv* env, jclass clazz, jbyteArray inName)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    string name;
    if (!arrayToString(env, inName, &name)) {
        return -1;
    }
    return appRepository->deleteConversation(name);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    listConversations
 * Signature: ()[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(listConversations) (JNIEnv* env, jclass clazz)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    list<string>* convNames = appRepository->listConversations();

    if (convNames == NULL)
        return NULL;

    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(convNames->size()), byteArrayClass, NULL);

    int32_t index = 0;
    while (!convNames->empty()) {
        string s = convNames->front();
        convNames->pop_front();
        jbyteArray retData = stringToArray(env, s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    insertEvent
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(insertEvent) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jbyteArray eventData)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -3;

    string name;
    if (!arrayToString(env, inName, &name)) {
        return -1;
    }
    string id;
    if (!arrayToString(env, eventId, &id)) {
        return -2;
    }
    string data;
    arrayToString(env, eventData, &data);
    return appRepository->insertEvent(name, id, data);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadEvent
 * Signature: ([B[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(loadEvent) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 2)
        return NULL;

    string name;
    if (!arrayToString(env, inName, &name)) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    string id;
    if (!arrayToString(env, eventId, &id)) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    int32_t msgNumber = 0;
    string data;
    int32_t result = appRepository->loadEvent(name, id, &data, &msgNumber);
    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        return NULL;
    }
    setReturnCode(env, code, result, msgNumber);
    jbyteArray retData = stringToArray(env, data);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadEventWithMsgId
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL 
JNI_FUNCTION(loadEventWithMsgId) (JNIEnv* env, jclass clazz, jbyteArray eventId, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    string id;
    if (!arrayToString(env, eventId, &id)) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    string data;
    int32_t result = appRepository->loadEventWithMsgId(id, &data);
    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        return NULL;
    }
    setReturnCode(env, code, result);
    jbyteArray retData = stringToArray(env, data);
    return retData;
}


/*
 * Class:     axolotl_AxolotlNative
 * Method:    existEvent
 * Signature: ([B[B)Z
 */
JNIEXPORT jboolean JNICALL
JNI_FUNCTION(existEvent) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return static_cast<jboolean>(false);

    string name;
    if (!arrayToString(env, inName, &name)) {
        return static_cast<jboolean>(false);
    }
    string id;
    if (!arrayToString(env, eventId, &id)) {
        return static_cast<jboolean>(false);
    }
    bool result = appRepository->existEvent(name, id);
    return static_cast<jboolean>(result);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadEvents
 * Signature: ([BII[I)[[B
 */
JNIEXPORT jobjectArray JNICALL 
JNI_FUNCTION(loadEvents) (JNIEnv* env, jclass clazz, jbyteArray inName, jint offset, jint number, jint direction, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 2)
        return NULL;

    string name;
    if (!arrayToString(env, inName, &name)) {
        setReturnCode(env, code, -1);
        return NULL;
    }

    int32_t msgNumber = 0;
    list<string*> events;
    int32_t result = appRepository->loadEvents(name, static_cast<uint32_t>(offset), number, direction, &events, &msgNumber);

    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        while (!events.empty()) {
            string* s = events.front();
            events.pop_front();
            delete s;
        }
        return NULL;
    }
    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(events.size()), byteArrayClass, NULL);

    int32_t index = 0;
    while (!events.empty()) {
        string* s = events.front();
        events.pop_front();
        jbyteArray retData = stringToArray(env, *s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
        delete s;
    }
    setReturnCode(env, code, result, msgNumber);
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    deleteEvent
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(deleteEvent) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    string name;
    if (!arrayToString(env, inName, &name)) {
        return -1;
    }
    string id;
    if (!arrayToString(env, eventId, &id)) {
        return -1;
    }
    return appRepository->deleteEvent(name, id);
}


/*
 * Class:     axolotl_AxolotlNative
 * Method:    insertObject
 * Signature: ([B[B[B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(insertObject) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jbyteArray objId, jbyteArray objData)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    string name;
    if (!arrayToString(env, inName, &name)) {
        return -1;
    }
    string event;
    if (!arrayToString(env, eventId, &event)) {
        return -1;
    }
    string id;
    if (!arrayToString(env, objId, &id)) {
        return -1;
    }
    string data;
    arrayToString(env, objData, &data);
    return appRepository->insertObject(name, event, id, data);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadObject
 * Signature: ([B[B[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(loadObject) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jbyteArray objId, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    string name;
    if (!arrayToString(env, inName, &name) || name.empty()) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    string event;
    if (!arrayToString(env, eventId, &event) || event.empty()) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    string id;
    if (!arrayToString(env, objId, &id) || id.empty()) {
        return NULL;
    }
    string data;
    int32_t result = appRepository->loadObject(name, event, id, &data);
    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        return NULL;
    }
    setReturnCode(env, code, result);
    jbyteArray retData = stringToArray(env, data);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    existObject
 * Signature: ([B[B[B)Z
 */
JNIEXPORT jboolean JNICALL
JNI_FUNCTION(existObject) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jbyteArray objId)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return static_cast<jboolean>(false);

    string name;
    if (!arrayToString(env, inName, &name) || name.empty()) {
        return static_cast<jboolean>(false);
    }
    string event;
    if (!arrayToString(env, eventId, &event) || event.empty()) {
        return static_cast<jboolean>(false);
    }
    string id;
    if (!arrayToString(env, objId, &id) || id.empty()) {
        return static_cast<jboolean>(false);
    }
    return static_cast<jboolean>(appRepository->existObject(name, event, id));
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadObjects
 * Signature: ([B[B[I)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(loadObjects) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    string name;
    if (!arrayToString(env, inName, &name) || name.empty()) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    string event;
    if (!arrayToString(env, eventId, &event) || event.empty()) {
        setReturnCode(env, code, -1);
        return NULL;
    }
    list<string*> objects;
    int32_t result = appRepository->loadObjects(name, event, &objects);

    if (SQL_FAIL(result)) {
        setReturnCode(env, code, result);
        while (!objects.empty()) {
            string* s = objects.front();
            objects.pop_front();
            delete s;
        }
        return NULL;
    }
    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(objects.size()), byteArrayClass, NULL);

    int32_t index = 0;
    while (!objects.empty()) {
        string* s = objects.front();
        objects.pop_front();
        jbyteArray retData = stringToArray(env, *s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
        delete s;
    }
    setReturnCode(env, code, result);
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    deleteObject
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL 
JNI_FUNCTION(deleteObject) (JNIEnv* env, jclass clazz, jbyteArray inName, jbyteArray eventId, jbyteArray objId)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    string name;
    if (!arrayToString(env, inName, &name) || name.empty()) {
        return -1;
    }
    string event;
    if (!arrayToString(env, eventId, &event) || event.empty()) {
        return -1;
    }
    string id;
    if (!arrayToString(env, objId, &id) || id.empty()) {
        return -1;
    }
    return appRepository->deleteObject(name, event, id);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    storeAttachmentStatus
 * Signature: ([B[BI)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(storeAttachmentStatus) (JNIEnv* env, jclass clazz, jbyteArray msgId, jbyteArray partnerName, jint status)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return 1;

    string messageId;
    if (!arrayToString(env, msgId, &messageId) || messageId.empty()) {
        return 1;    // 1 is the generic SQL error code
    }
    string pn;
    if (partnerName != NULL) {
        arrayToString(env, partnerName, &pn);
    }
    return appRepository->storeAttachmentStatus(messageId, pn, status);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    deleteAttachmentStatus
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(deleteAttachmentStatus) (JNIEnv* env, jclass clazz, jbyteArray msgId, jbyteArray partnerName)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return 1;

    string messageId;
    if (!arrayToString(env, msgId, &messageId) || messageId.empty()) {
        return 1;    // 1 is the generic SQL error code
    }
    string pn;
    if (partnerName != NULL) {
        arrayToString(env, partnerName, &pn);
    }
    return appRepository->deleteAttachmentStatus(messageId, pn);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    deleteWithAttachmentStatus
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(deleteWithAttachmentStatus) (JNIEnv* env, jclass clazz, jint status)
{
    (void)clazz;
    (void)env;

    if (!IS_APP_REPO_OPEN)
        return 1;

    return appRepository->deleteWithAttachmentStatus(status);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadAttachmentStatus
 * Signature: ([B[B[I)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(loadAttachmentStatus) (JNIEnv* env, jclass clazz, jbyteArray msgId, jbyteArray partnerName, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return -1;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return -1;

    string messageId;
    if (!arrayToString(env, msgId, &messageId) || messageId.empty()) {
        setReturnCode(env, code, 1);   // 1 is the generic SQL error code
        return -1;
    }
    string pn;
    if (partnerName != NULL) {
        arrayToString(env, partnerName, &pn);
    }
    int32_t status;
    int32_t result = appRepository->loadAttachmentStatus(messageId, pn, &status);
    setReturnCode(env, code, result);
    return status;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadMsgsIdsWithAttachmentStatus
 * Signature: (I[I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(loadMsgsIdsWithAttachmentStatus) (JNIEnv* env, jclass clazz, jint status, jintArray code)
{
    (void)clazz;

    if (!IS_APP_REPO_OPEN)
        return NULL;

    if (code == NULL || env->GetArrayLength(code) < 1)
        return NULL;

    list<string> msgIds;
    int32_t result = appRepository->loadMsgsIdsWithAttachmentStatus(status, &msgIds);

    jclass stringArrayClass = env->FindClass("java/lang/String");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(msgIds.size()), stringArrayClass, NULL);

    int32_t index = 0;
    while (!msgIds.empty()) {
        string s = msgIds.front();
        msgIds.pop_front();
        jstring stringData = env->NewStringUTF(s.c_str());
        env->SetObjectArrayElement(retArray, index++, stringData);
        env->DeleteLocalRef(stringData);
    }
    setReturnCode(env, code, result);
    return retArray;
}


static uint8_t* jarrayToCarray(JNIEnv* env, jbyteArray array, size_t* len)
{
    *len = 0;
    if (array == NULL)
        return NULL;

    int tmpLen = env->GetArrayLength(array);
    if (tmpLen <= 0)
        return NULL;

    size_t dataLen = static_cast<size_t>(tmpLen);
    const uint8_t* tmp = (uint8_t*)env->GetByteArrayElements(array, 0);
    if (tmp == NULL)
        return NULL;

    uint8_t* buffer = (uint8_t*)malloc(dataLen);
    if (buffer == NULL)
        return NULL;

    *len = dataLen;
    memcpy(buffer, tmp, dataLen);
    env->ReleaseByteArrayElements(array, (jbyte*)tmp, 0);
    return buffer;
}


/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptNew
 * Signature: ([B[B[B[I)J
 
 byte[] context, byte[] data, byte[] metaData, int[] errorCode
 */
JNIEXPORT jlong JNICALL
JNI_FUNCTION(cloudEncryptNew) (JNIEnv* env, jclass clazz, jbyteArray context, jbyteArray data, jbyteArray metaData, jintArray code)
{
    (void)clazz;

    SCloudContextRef scCtxEnc;

    setReturnCode(env, code, kSCLError_NoErr);

    // cloudFree calls free() to return the malloc'd data buffers created by jarrayToCarray
    size_t ctxLen;
    uint8_t* ctx = jarrayToCarray(env, context, &ctxLen);

    size_t dataLen;
    uint8_t* inData = jarrayToCarray(env, data, &dataLen);
    if (inData == NULL || dataLen == 0) {
        setReturnCode(env, code, kSCLError_BadParams);
        return 0L;
    }
    size_t metaLen;
    uint8_t* inMetaData = jarrayToCarray(env, metaData, &metaLen);
    if (inMetaData == NULL || metaLen == 0) {
        setReturnCode(env, code, kSCLError_BadParams);
        return 0L;
    }
    SCLError err = SCloudEncryptNew(ctx, ctxLen, (void*)inData, dataLen, (void*)inMetaData, metaLen,
                                    NULL, NULL, &scCtxEnc);
    if (err != kSCLError_NoErr) {
        setReturnCode(env, code, err);
        return 0L;
    }
    jlong retval = (jlong)scCtxEnc;
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudCalculateKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(cloudCalculateKey) (JNIEnv* env, jclass clazz, jlong cloudRef)
{
    (void)clazz;
    (void)env;

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    SCLError err = SCloudCalculateKey(scCtxEnc, 0);
    return err;
}

static jbyteArray cArrayToJArray(JNIEnv* env, const uint8_t* input, size_t len)
{
    if (len == 0)
        return NULL;

    jbyteArray data = env->NewByteArray(static_cast<jsize>(len));
    if (data == NULL)
        return NULL;
    env->SetByteArrayRegion(data, 0, static_cast<jsize>(len), (jbyte*)input);
    return data;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptGetKeyBLOB
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudEncryptGetKeyBLOB) (JNIEnv* env, jclass clazz, jlong cloudRef, jintArray code)
{
    (void)clazz;

    SCLError err;
    uint8_t* blob = NULL;
    size_t blobSize = 0;

    setReturnCode(env, code, kSCLError_NoErr);

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    err = SCloudEncryptGetKeyBLOB( scCtxEnc, &blob, &blobSize);

    if (err != kSCLError_NoErr) {
        setReturnCode(env, code, err);
        if (blob != NULL)
            free(blob);
        return NULL;
    }
    jbyteArray retval = cArrayToJArray(env, blob, blobSize);
    free(blob);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptGetSegmentBLOB
 * Signature: (JI[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudEncryptGetSegmentBLOB) (JNIEnv* env, jclass clazz, jlong cloudRef, jint segNum, jintArray code)
{
    (void)clazz;

    SCLError err;
    uint8_t* blob = NULL;
    size_t blobSize = 0;

    setReturnCode(env, code, kSCLError_NoErr);

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    err = SCloudEncryptGetSegmentBLOB(scCtxEnc, segNum, &blob, &blobSize);

    if (err != kSCLError_NoErr) {
        setReturnCode(env, code, err);
        if (blob != NULL)
            free(blob);
        return NULL;
    }
    jbyteArray retval = cArrayToJArray(env, blob, blobSize);
    free(blob);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptGetLocator
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudEncryptGetLocator) (JNIEnv* env, jclass clazz, jlong cloudRef, jintArray code)
{
    (void)clazz;

    SCLError err;
    uint8_t buffer[1024];
    size_t bufSize = 1024;

    setReturnCode(env, code, kSCLError_NoErr);

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    err = SCloudEncryptGetLocator(scCtxEnc, buffer, &bufSize);
    if (err != kSCLError_NoErr) {
        setReturnCode(env, code, err);
        return NULL;
    }
    jbyteArray retval = cArrayToJArray(env, buffer, bufSize);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptGetLocatorREST
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudEncryptGetLocatorREST) (JNIEnv* env, jclass clazz, jlong cloudRef, jintArray code)
{
    (void)clazz;

    SCLError err;
    uint8_t buffer[1024];
    size_t bufSize = 1024;

    setReturnCode(env, code, kSCLError_NoErr);

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    err = SCloudEncryptGetLocatorREST(scCtxEnc, buffer, &bufSize);
    if (err != kSCLError_NoErr) {
        setReturnCode(env, code, err);
        return NULL;
    }
    jbyteArray retval = cArrayToJArray(env, buffer, bufSize);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudEncryptNext
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudEncryptNext) (JNIEnv* env, jclass clazz, jlong cloudRef, jintArray code)
{
    (void)clazz;

    SCLError err;

    SCloudContextRef scCtxEnc = (SCloudContextRef)cloudRef;

    size_t required = SCloudEncryptBufferSize(scCtxEnc);
    jbyteArray data = env->NewByteArray(static_cast<jsize>(required));
    if (data == NULL) {
        setReturnCode(env, code, kSCLError_OutOfMemory);
        return NULL;
    }

    uint8_t* bigBuffer = (uint8_t*)env->GetByteArrayElements(data, 0);
    err = SCloudEncryptNext(scCtxEnc, bigBuffer, &required);
    setReturnCode(env, code, err);

    env->ReleaseByteArrayElements(data, (jbyte*)bigBuffer, 0);
    return data;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudDecryptNew
 * Signature: ([B[I)J
 */
JNIEXPORT jlong JNICALL
JNI_FUNCTION(cloudDecryptNew) (JNIEnv* env, jclass clazz, jbyteArray key, jintArray code)
{
    (void)clazz;
    (void)code;

    SCloudContextRef scCtxDec;

    string keyIn;
    if (!arrayToString(env, key, &keyIn))
        return 0L;

    SCloudDecryptNew((uint8_t*)keyIn.data(), keyIn.size(), NULL, NULL, &scCtxDec);
    jlong retval = (jlong)scCtxDec;
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudDecryptNext
 * Signature: (J[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(cloudDecryptNext) (JNIEnv* env, jclass clazz, jlong cloudRef, jbyteArray in)
{
    (void)clazz;

    SCLError err;
    SCloudContextRef scCtxDec = (SCloudContextRef)cloudRef;

    int tmpLen = env->GetArrayLength(in);
    if (tmpLen <= 0)
        return kSCLError_BadParams;

    size_t dataLen = static_cast<size_t>(tmpLen);
    uint8_t* data = (uint8_t*)env->GetByteArrayElements(in, 0);
    if (data == NULL) {
        return kSCLError_OutOfMemory;
    }
    err = SCloudDecryptNext(scCtxDec, data, dataLen);
    env->ReleaseByteArrayElements(in, (jbyte*)data, 0);
    return err;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudGetDecryptedData
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudGetDecryptedData) (JNIEnv* env, jclass clazz, jlong cloudRef)
{
    (void)clazz;

    SCloudContextRef scCtxDec = (SCloudContextRef)cloudRef;

    uint8_t* dataBuffer = NULL;
    uint8_t* metaBuffer = NULL;
    size_t dataLen;
    size_t metaLen;

    SCloudDecryptGetData(scCtxDec, &dataBuffer, &dataLen, &metaBuffer, &metaLen);

    jbyteArray retval = cArrayToJArray(env, dataBuffer, dataLen);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudGetDecryptedMetaData
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(cloudGetDecryptedMetaData) (JNIEnv* env, jclass clazz, jlong cloudRef)
{
    (void)clazz;

    SCloudContextRef scCtxDec = (SCloudContextRef)cloudRef;

    uint8_t* dataBuffer = NULL;
    uint8_t* metaBuffer = NULL;
    size_t dataLen;
    size_t metaLen;

    SCloudDecryptGetData(scCtxDec, &dataBuffer, &dataLen, &metaBuffer, &metaLen);

    jbyteArray retval = cArrayToJArray(env, metaBuffer, metaLen);
    return retval;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    cloudFree
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
JNI_FUNCTION(cloudFree) (JNIEnv* env, jclass clazz, jlong cloudRef)
{
    (void)clazz;
    (void)env;

    SCloudContextRef scCtx = (SCloudContextRef)cloudRef;
    SCloudFree(scCtx, 1);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getUid
 * Signature: (Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
JNI_FUNCTION(getUid)(JNIEnv* env, jclass clazz, jstring alias, jbyteArray authorization)
{
    (void)clazz;

    string auth;
    if (!arrayToString(env, authorization, &auth) || auth.empty()) {
        if (axoAppInterface == NULL)
            return NULL;
        auth = axoAppInterface->getOwnAuthrization();
    }
    if (alias == NULL) {
        return NULL;
    }
    const char* aliasTmp = env->GetStringUTFChars(alias, 0);
    string aliasString(aliasTmp);
    env->ReleaseStringUTFChars(alias, aliasTmp);
    if (aliasString.empty())
        return NULL;

    NameLookup* nameCache = NameLookup::getInstance();
    string uid = nameCache->getUid(aliasString, auth);

    if (uid.empty())
        return NULL;

    jstring uidJava = env->NewStringUTF(uid.c_str());
    return uidJava;
}

static jbyteArray getUserInfoInternal(JNIEnv* env, jstring alias, jbyteArray authorization, bool cacheOnly, int32_t* errorCode)
{
    string auth;
    if (!arrayToString(env, authorization, &auth) || auth.empty()) {
        if (axoAppInterface == NULL) {
            *errorCode = GENERIC_ERROR;
            return NULL;
        }
        auth = axoAppInterface->getOwnAuthrization();
    }
    if (alias == NULL) {
        *errorCode = GENERIC_ERROR;
        return NULL;
    }
    const char* aliasTmp = env->GetStringUTFChars(alias, 0);
    string aliasString(aliasTmp);
    env->ReleaseStringUTFChars(alias, aliasTmp);
    if (aliasString.empty()) {
        *errorCode = GENERIC_ERROR;
        return NULL;
    }

    NameLookup* nameCache = NameLookup::getInstance();
    shared_ptr<UserInfo> userInfo = nameCache->getUserInfo(aliasString, auth, cacheOnly, errorCode);

    if (!userInfo)
        return NULL;

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "uid", userInfo->uniqueId.c_str());
    cJSON_AddStringToObject(root, "display_name", userInfo->displayName.c_str());
    cJSON_AddStringToObject(root, "alias0", userInfo->alias0.c_str());
    cJSON_AddStringToObject(root, "lookup_uri", userInfo->contactLookupUri.c_str());
    cJSON_AddStringToObject(root, "avatar_url", userInfo->avatarUrl.c_str());

    char *out = cJSON_PrintUnformatted(root);
    string json(out);
    cJSON_Delete(root); free(out);

    jbyteArray retData = stringToArray(env, json);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getUserInfo
 * Signature: (Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getUserInfo)(JNIEnv* env, jclass clazz, jstring alias, jbyteArray authorization, jintArray code)
{
    (void)clazz;
    int32_t errorCode = 0;

    jbyteArray retData = getUserInfoInternal(env, alias, authorization, false, &errorCode);

    if (code != NULL && env->GetArrayLength(code) >= 1) {
        setReturnCode(env, code, errorCode);
    }
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getUserInfoFromCache
 * Signature: (Ljava/lang/String;[B)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getUserInfoFromCache)(JNIEnv* env, jclass clazz, jstring alias)
{
    (void)clazz;
    int32_t errorCode = 0;
    return getUserInfoInternal(env, alias, NULL, true, &errorCode);
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    refreshUserData
 * Signature: (Ljava/lang/String;[B)[B
 */
JNIEXPORT jbyteArray
JNICALL JNI_FUNCTION(refreshUserData)(JNIEnv* env, jclass clazz, jstring alias, jbyteArray authorization)
{
    string auth;
    if (!arrayToString(env, authorization, &auth) || auth.empty()) {
        if (axoAppInterface == NULL)
            return NULL;
        auth = axoAppInterface->getOwnAuthrization();
    }
    if (alias == NULL) {
        return NULL;
    }
    const char* aliasTmp = env->GetStringUTFChars(alias, 0);
    string aliasString(aliasTmp);
    env->ReleaseStringUTFChars(alias, aliasTmp);
    if (aliasString.empty())
        return NULL;

    NameLookup* nameCache = NameLookup::getInstance();
    shared_ptr<UserInfo> userInfo = nameCache->refreshUserData(aliasString, auth);

    if (!userInfo)
        return NULL;

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "uid", userInfo->uniqueId.c_str());
    cJSON_AddStringToObject(root, "display_name", userInfo->displayName.c_str());
    cJSON_AddStringToObject(root, "alias0", userInfo->alias0.c_str());
    cJSON_AddStringToObject(root, "lookup_uri", userInfo->contactLookupUri.c_str());
    cJSON_AddStringToObject(root, "avatar_url", userInfo->avatarUrl.c_str());

    char *out = cJSON_PrintUnformatted(root);
    string json(out);
    cJSON_Delete(root); free(out);

    jbyteArray retData = stringToArray(env, json);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getAliases
 * Signature: (Ljava/lang/String;[B)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(getAliases)(JNIEnv* env, jclass clazz, jstring uuid)
{
    (void)clazz;

    if (uuid == NULL) {
        return NULL;
    }
    const char* uuidTemp = env->GetStringUTFChars(uuid, 0);
    string uuidString(uuidTemp);
    env->ReleaseStringUTFChars(uuid, uuidTemp);
    if (uuidString.empty())
        return NULL;

    NameLookup* nameCache = NameLookup::getInstance();
    shared_ptr<list<string> > aliases = nameCache->getAliases(uuidString);
    if (!aliases)
        return NULL;
    size_t size = aliases->size();
    if (size == 0)
        return NULL;

    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(size), byteArrayClass, NULL);

    int32_t index = 0;
    while (!aliases->empty()) {
        string s = aliases->front();
        aliases->pop_front();
        jbyteArray retData = stringToArray(env, s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    return retArray;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    addAliasToUuid
 * Signature: (Ljava/lang/String;Ljava/lang/String;[B[B)I
 */
JNIEXPORT jint JNICALL
JNI_FUNCTION(addAliasToUuid)(JNIEnv* env, jclass clazz, jstring alias, jstring uuid, jbyteArray userData)
{
    (void)clazz;

    if (uuid == NULL) {
        return NameLookup::MissingParameter;
    }
    const char* uuidTemp = env->GetStringUTFChars(uuid, 0);
    string uuidString(uuidTemp);
    env->ReleaseStringUTFChars(uuid, uuidTemp);
    if (uuidString.empty())
        return NameLookup::MissingParameter;

    if (alias == NULL) {
        return NameLookup::MissingParameter;
    }
    const char* aliasTmp = env->GetStringUTFChars(alias, 0);
    string aliasString(aliasTmp);
    env->ReleaseStringUTFChars(alias, aliasTmp);
    if (aliasString.empty())
        return NameLookup::MissingParameter;

    string data;
    if (!arrayToString(env, userData, &data))
        return NameLookup::MissingParameter;

    NameLookup* nameCache = NameLookup::getInstance();
    NameLookup::AliasAdd ret = nameCache->addAliasToUuid(aliasString, uuidString, data);
    return ret;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    getDisplayName
 * Signature: (Ljava/lang/String;[B)[B
 */
JNIEXPORT jbyteArray JNICALL
JNI_FUNCTION(getDisplayName)(JNIEnv* env, jclass clazz, jstring uuid)
{
    (void)clazz;

    if (uuid == NULL) {
        return NULL;
    }
    const char* uuidTemp = env->GetStringUTFChars(uuid, 0);
    string uuidString(uuidTemp);
    env->ReleaseStringUTFChars(uuid, uuidTemp);
    if (uuidString.empty())
        return NULL;

    NameLookup* nameCache = NameLookup::getInstance();
    shared_ptr<string> displayName = nameCache->getDisplayName(uuidString);
    if (!displayName)
        return NULL;
    jbyteArray retData = stringToArray(env, *displayName);
    return retData;
}

/*
 * Class:     axolotl_AxolotlNative
 * Method:    loadCapturedMsgs
 * Signature: ([B[B[B[I)[[B
 */
JNIEXPORT jobjectArray JNICALL
JNI_FUNCTION(loadCapturedMsgs)(JNIEnv* env, jclass clazz, jbyteArray name, jbyteArray msgId, jbyteArray devId, jintArray code)
{
    (void)clazz;

    string nameString;
    arrayToString(env, name, &nameString);

    string msgIdString;
    arrayToString(env, msgId, &msgIdString);

    string devIdString;
    arrayToString(env, devId, &devIdString);

    int32_t errorCode;
    SQLiteStoreConv* store = SQLiteStoreConv::getStore();
    shared_ptr<list<string> > records = store->loadMsgTrace(nameString, msgIdString, devIdString, &errorCode);

    if (code != NULL && env->GetArrayLength(code) >= 1) {
        setReturnCode(env, code, errorCode);
    }
    jclass byteArrayClass = env->FindClass("[B");
    jobjectArray retArray = env->NewObjectArray(static_cast<jsize>(records->size()), byteArrayClass, NULL);

    int32_t index = 0;
    while (!records->empty()) {
        string s = records->front();
        records->pop_front();
        jbyteArray retData = stringToArray(env, s);
        env->SetObjectArrayElement(retArray, index++, retData);
        env->DeleteLocalRef(retData);
    }
    return retArray;
}
