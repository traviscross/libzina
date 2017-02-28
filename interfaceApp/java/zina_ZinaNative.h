/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class zina_ZinaNative */

#ifndef _Included_zina_ZinaNative
#define _Included_zina_ZinaNative
#ifdef __cplusplus
extern "C" {
#endif
#undef zina_ZinaNative_DEVICE_SCAN
#define zina_ZinaNative_DEVICE_SCAN 1L
/*
 * Class:     zina_ZinaNative
 * Method:    doInit
 * Signature: (ILjava/lang/String;[B[B[B[BLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_doInit
  (JNIEnv *, jobject, jint, jstring, jbyteArray, jbyteArray, jbyteArray, jbyteArray, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    prepareMessage
 * Signature: ([B[B[BZ[I)[Lzina/ZinaNative/PreparedMessageData;
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_prepareMessage
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jboolean, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    prepareMessageToSiblings
 * Signature: ([B[B[BZ[I)[Lzina/ZinaNative/PreparedMessageData;
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_prepareMessageToSiblings
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jboolean, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    doSendMessages
 * Signature: ([J)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_doSendMessages
  (JNIEnv *, jclass, jlongArray);

/*
 * Class:     zina_ZinaNative
 * Method:    removePreparedMessages
 * Signature: ([J)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_removePreparedMessages
  (JNIEnv *, jclass, jlongArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getKnownUsers
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getKnownUsers
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    getOwnIdentityKey
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getOwnIdentityKey
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    getIdentityKeys
 * Signature: ([B)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_getIdentityKeys
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getZinaDevicesUser
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getZinaDevicesUser
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    registerZinaDevice
 * Signature: ([I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_registerZinaDevice
  (JNIEnv *, jclass, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    removeZinaDevice
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_removeZinaDevice
  (JNIEnv *, jclass, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    newPreKeys
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_newPreKeys
  (JNIEnv *, jclass, jint);

/*
 * Class:     zina_ZinaNative
 * Method:    getNumPreKeys
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_getNumPreKeys
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    getErrorCode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_getErrorCode
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    getErrorInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_zina_ZinaNative_getErrorInfo
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    testCommand
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_testCommand
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    zinaCommand
 * Signature: (Ljava/lang/String;[B[I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_zina_ZinaNative_zinaCommand
  (JNIEnv *, jclass, jstring, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    createNewGroup
 * Signature: ([B[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_zina_ZinaNative_createNewGroup
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    modifyGroupSize
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_modifyGroupSize
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     zina_ZinaNative
 * Method:    setGroupName
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_setGroupName
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    setGroupBurnTime
 * Signature: (Ljava/lang/String;JI)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_setGroupBurnTime
  (JNIEnv *, jclass, jstring, jlong, jint);

/*
 * Class:     zina_ZinaNative
 * Method:    setGroupAvatar
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_setGroupAvatar
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    listAllGroups
 * Signature: ([I)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_listAllGroups
  (JNIEnv *, jclass, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getGroup
 * Signature: (Ljava/lang/String;[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getGroup
  (JNIEnv *, jclass, jstring, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getAllGroupMembers
 * Signature: (Ljava/lang/String;[I)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_getAllGroupMembers
  (JNIEnv *, jclass, jstring, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getGroupMember
 * Signature: (Ljava/lang/String;[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getGroupMember
  (JNIEnv *, jclass, jstring, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    addUser
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_addUser
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    removeUserFromAddUpdate
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_removeUserFromAddUpdate
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cancelGroupChangeSet
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_cancelGroupChangeSet
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    applyGroupChangeSet
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_applyGroupChangeSet
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    sendGroupMessage
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_sendGroupMessage
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    leaveGroup
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_leaveGroup
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    removeUser
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_removeUser
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    removeUserFromRemoveUpdate
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_removeUserFromRemoveUpdate
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    groupMessageRemoved
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_groupMessageRemoved
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    groupsSyncSibling
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_groupsSyncSibling
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    groupSyncSibling
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_groupSyncSibling
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    requestGroupsSync
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_requestGroupsSync
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    repoOpenDatabase
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_repoOpenDatabase
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    repoCloseDatabase
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_repoCloseDatabase
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    repoIsOpen
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_repoIsOpen
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    existConversation
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_existConversation
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    storeConversation
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_storeConversation
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadConversation
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_loadConversation
  (JNIEnv *, jclass, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteConversation
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteConversation
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    listConversations
 * Signature: ()[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_listConversations
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    insertEvent
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_insertEvent
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadEvent
 * Signature: ([B[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_loadEvent
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadEventWithMsgId
 * Signature: ([B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_loadEventWithMsgId
  (JNIEnv *, jclass, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    existEvent
 * Signature: ([B[B)Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_existEvent
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadEvents
 * Signature: ([BIII[I)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_loadEvents
  (JNIEnv *, jclass, jbyteArray, jint, jint, jint, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteEvent
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteEvent
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteAllEvents
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteAllEvents
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    insertObject
 * Signature: ([B[B[B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_insertObject
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadObject
 * Signature: ([B[B[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_loadObject
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    existObject
 * Signature: ([B[B[B)Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_existObject
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadObjects
 * Signature: ([B[B[I)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_loadObjects
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteObject
 * Signature: ([B[B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteObject
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    storeAttachmentStatus
 * Signature: ([B[BI)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_storeAttachmentStatus
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jint);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteAttachmentStatus
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteAttachmentStatus
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    deleteWithAttachmentStatus
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_deleteWithAttachmentStatus
  (JNIEnv *, jclass, jint);

/*
 * Class:     zina_ZinaNative
 * Method:    loadAttachmentStatus
 * Signature: ([B[B[I)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_loadAttachmentStatus
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    loadMsgsIdsWithAttachmentStatus
 * Signature: (I[I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_loadMsgsIdsWithAttachmentStatus
  (JNIEnv *, jclass, jint, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptNew
 * Signature: ([B[B[B[I)J
 */
JNIEXPORT jlong JNICALL Java_zina_ZinaNative_cloudEncryptNew
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudCalculateKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_cloudCalculateKey
  (JNIEnv *, jclass, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptGetKeyBLOB
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudEncryptGetKeyBLOB
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptGetSegmentBLOB
 * Signature: (JI[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudEncryptGetSegmentBLOB
  (JNIEnv *, jclass, jlong, jint, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptGetLocator
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudEncryptGetLocator
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptGetLocatorREST
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudEncryptGetLocatorREST
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudEncryptNext
 * Signature: (J[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudEncryptNext
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudDecryptNew
 * Signature: ([B[I)J
 */
JNIEXPORT jlong JNICALL Java_zina_ZinaNative_cloudDecryptNew
  (JNIEnv *, jclass, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudDecryptNext
 * Signature: (J[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_cloudDecryptNext
  (JNIEnv *, jclass, jlong, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudGetDecryptedData
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudGetDecryptedData
  (JNIEnv *, jclass, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudGetDecryptedMetaData
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_cloudGetDecryptedMetaData
  (JNIEnv *, jclass, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    cloudFree
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_cloudFree
  (JNIEnv *, jclass, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    getUid
 * Signature: (Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_zina_ZinaNative_getUid
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getUserInfo
 * Signature: (Ljava/lang/String;[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getUserInfo
  (JNIEnv *, jclass, jstring, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    refreshUserData
 * Signature: (Ljava/lang/String;[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_refreshUserData
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getUserInfoFromCache
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getUserInfoFromCache
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    getAliases
 * Signature: (Ljava/lang/String;)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_getAliases
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    addAliasToUuid
 * Signature: (Ljava/lang/String;Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_addAliasToUuid
  (JNIEnv *, jclass, jstring, jstring, jbyteArray);

/*
 * Class:     zina_ZinaNative
 * Method:    getDisplayName
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_zina_ZinaNative_getDisplayName
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    loadCapturedMsgs
 * Signature: ([B[B[B[I)[[B
 */
JNIEXPORT jobjectArray JNICALL Java_zina_ZinaNative_loadCapturedMsgs
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jintArray);

/*
 * Class:     zina_ZinaNative
 * Method:    sendDrMessageData
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_sendDrMessageData
  (JNIEnv *, jclass, jstring, jstring, jstring, jlong, jlong, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    sendDrMessageMetadata
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJ)V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_sendDrMessageMetadata
  (JNIEnv *, jclass, jstring, jstring, jstring, jlong, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    sendDrInCircleCallMetadata
 * Signature: (Ljava/lang/String;ZLjava/lang/String;JJ)V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_sendDrInCircleCallMetadata
  (JNIEnv *, jclass, jstring, jboolean, jstring, jlong, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    sendDrSilentWorldCallMetadata
 * Signature: (Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;JJ)V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_sendDrSilentWorldCallMetadata
  (JNIEnv *, jclass, jstring, jboolean, jstring, jstring, jlong, jlong);

/*
 * Class:     zina_ZinaNative
 * Method:    processPendingDrRequests
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_zina_ZinaNative_processPendingDrRequests
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    isDrEnabled
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_isDrEnabled
  (JNIEnv *, jclass);

/*
 * Class:     zina_ZinaNative
 * Method:    isDrEnabledForUser
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_zina_ZinaNative_isDrEnabledForUser
  (JNIEnv *, jclass, jstring);

/*
 * Class:     zina_ZinaNative
 * Method:    setDataRetentionFlags
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_zina_ZinaNative_setDataRetentionFlags
  (JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif
#endif
