# Copyright 2016 Silent Circle, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.1)

set(AUTO_BUILD $ENV{AUTOMATED_BUILD})

# If running auto build then we assume that we build it for Android, thus we
# don't need a CC or CXX compiler on the build system. It's all done in Android's
# cross-compiler environment when calling ndk-build. Ignore some Cmake messages
# in this case.
if (AUTO_BUILD EQUAL 1)
    PROJECT(libzina NONE)
else()
    PROJECT(libzina)
endif()

SET(CPACK_PACKAGE_VERSION_MAJOR 1)
SET(CPACK_PACKAGE_VERSION_MINOR 0)
SET(CPACK_PACKAGE_VERSION_PATCH 0)

set (VERSION 1.0.0)
set (SOVERSION 1)

### Configuration section
# Where to find the modules that we share with ZRTP, i.e. the ZRTP root directory
if (AUTO_BUILD EQUAL 1)
    set (ZRTP_BASE_DIR $ENV{WORKSPACE}/silentphone2/support/zrtpcpp)
elseif (NOT "$ENV{ZRTP_BASE_DIR}" STREQUAL "")
    set (ZRTP_BASE_DIR $ENV{ZRTP_BASE_DIR})
else()
    set (ZRTP_BASE_DIR ${CMAKE_SOURCE_DIR}/../zrtpcpp)
endif()

option(STANDALONE "Build standlone shared library, otherwise a static lib without ZRTP crypto modules." OFF)
option(UNITTESTS "Build unit tests, implies STANDALONE true." OFF)
option(ANDROID "Compile and build for Android." OFF)
option(EMSCRIPTEN "Compile for an emscripten target" OFF)
option(UBSAN "Compile with Clang UndefinedBehaviorSanitizer (UBSAN)" OFF)
option(ASAN "Compile with Clang AddressSanitizer (ASAN)" OFF)
option(TSAN "Compile with Clang ThreadSanitizer (TSAN)" OFF)
option(SSSAN "Compile with Clang SafeStack (SSSAN)" OFF)
option(CFISAN "Compile with Clang Control Flow Integrity (CFISAN)" OFF)

set(LIBRARY_BUILD_TYPE SHARED)
set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (UNITTESTS)
    set(STANDALONE true)
endif()

if (NOT STANDALONE)
    set(LIBRARY_BUILD_TYPE STATIC)
endif()

if (UBSAN OR ASAN OR TSAN OR SSSAN OR CFISAN)
    if (  NOT AUTO_BUILD AND
          NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" AND
          NOT CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR "Clang is required to build with SAN; set CC=clang")
    endif()
    if (  NOT AUTO_BUILD AND
          NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
          NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR "Clang is required to build with SAN; set CXX=clang")
    endif()
endif()

if (UBSAN)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "SAN set: exit on detected undefined behavior (no runtime, fast)")
        set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=undefined")
        set(SAN_FLAGS "${SAN_FLAGS} -fno-sanitize-recover=undefined")
        set(SAN_FLAGS "${SAN_FLAGS} -fsanitize-trap=undefined")
        if (AUTO_BUILD OR NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
            message(STATUS "SAN opt: sanitize nullability")
            set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=nullability")
            set(SAN_FLAGS "${SAN_FLAGS} -fno-sanitize-recover=nullability")
            set(SAN_FLAGS "${SAN_FLAGS} -fsanitize-trap=nullability")
        endif()
        #set(SAN_FLAGS "${SAN_FLAGS} -fsanitize-minimal-runtime") # requires clang-6.0
    else()
        message(STATUS "SAN set: exit on detected undefined behavior (fast)")
        set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=undefined")
        set(SAN_FLAGS "${SAN_FLAGS} -fno-sanitize-recover=undefined")
        if (AUTO_BUILD OR NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0)
            message(STATUS "SAN opt: sanitize nullability")
            set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=nullability")
            set(SAN_FLAGS "${SAN_FLAGS} -fno-sanitize-recover=nullability")
        endif()
    endif()
endif()

if (ASAN)
    message(STATUS "SAN set: exit on detected memory errors (slow)")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(WARNING "ASAN enabled on release build may be slow")
    endif()
    set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=address")
    if (AUTO_BUILD OR NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.0)
        message(STATUS "SAN opt: sanitize address use after scope")
        set(SAN_FLAGS "${SAN_FLAGS} -fsanitize-address-use-after-scope")
    endif()
endif()

if (TSAN)
    if(ASAN)
        message(FATAL_ERROR "Cannot enable TSAN with ASAN")
    endif()
    message(STATUS "SAN set: exit on detected thread errors (slow)")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(WARNING "TSAN enabled on release build may be slow")
    endif()
    set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=thread")
endif()

if (SSSAN)
    message(STATUS "SAN set: exit on detected stack buffer overflows (fast)")
    set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=safe-stack")
endif()

if (CFISAN)
    message(STATUS "SAN set: exit on detected control flow integrity violations (fast)")
    set(SAN_FLAGS "${SAN_FLAGS} -fsanitize=cfi -flto -fvisibility=hidden")
    if (AUTO_BUILD OR NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 3.9)
        set(SAN_FLAGS "${SAN_FLAGS} -fvisibility=hidden")
    endif()
    set(SAN_FLAGS "${SAN_FLAGS} -fsanitize-cfi-cross-dso")
endif()

if (SAN_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${SAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SAN_FLAGS}")
    set(CMAKE_C_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${SAN_FLAGS}")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${SAN_FLAGS}")
endif()

set(zinaLibName zina)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

### No more configuration below this line (usually ;-) )
if (EXISTS "${ZRTP_BASE_DIR}/zrtp/crypto/hmac256.cpp")
    message(STATUS "Using ${ZRTP_BASE_DIR} for common modules")
else()
    message(FATAL_ERROR "Cannot locate ZRTP_BASE_DIR: ${ZRTP_BASE_DIR}. Please set correct path.")
endif()


string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if ("debug" STREQUAL build_type)
    set (LOG_MAX_LEVEL "-DLOG_MAX_LEVEL=VERBOSE")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLOG_MAX_LEVEL=VERBOSE")
else()
    set (LOG_MAX_LEVEL "-DLOG_MAX_LEVEL=WARNING")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLOG_MAX_LEVEL=WARNING")
endif()


# use protobuf_lite.pc
# include most of the fine stuff we need
include(FindPkgConfig)
include(CheckIncludeFiles)

find_package(JNI)
if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
    include_directories(${JNI_INCLUDE_DIRS})
endif()

if (NOT ANDROID)
    pkg_check_modules(PROTO protobuf-lite>=2.6.1)
    if (PROTO_FOUND)
        include_directories(${PROTO_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${PROTO_LDFLAGS})
    else()
        message(FATAL_ERROR "ProtocolBuffer library not found")
    endif()

    pkg_check_modules(SQLCIPHER sqlcipher>=3.4)
    if (SQLCIPHER_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SQLCIPHER_INCLUDE_DIRS})
        check_include_files(sqlite3.h HAVE_SQLCIPHER_H)
        include_directories(${SQLCIPHER_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${SQLCIPHER_LDFLAGS})
        set(LIBS ${LIBS} -lsqlcipher)
        MESSAGE(STATUS "Using SQlCipher based ZRTP cache")
        add_definitions(-DSQL_CIPHER -DSQLITE_HAS_CODEC)
    else()
        message(FATAL_ERROR "SQLCipher library not found")
    endif()
endif()

# pkg_check_modules(SQLITE3 sqlite3>=3.7)
# if (SQLITE3_FOUND)
# #    check_include_files(sqlite3.h HAVE_SQLITE_H)
#     set(LIBS ${LIBS} -lsqlite3)
#     MESSAGE(STATUS "Using SQLite based ZRTP cache")
# else()
#     message(FATAL_ERROR "SQLite3 library not found")
# endif()

if (UNITTESTS)
    add_definitions(-DUNITTESTS)
else()
    add_definitions(-DEMBEDDED)
endif()

add_subdirectory(ratchet/crypto)

set (protocol_src
    ratchet/ZinaZrtpConnector.cpp
    ratchet/ZinaPreKeyConnector.cpp
    ratchet/ratchet/ZinaRatchet.cpp
    ratchet/state/ZinaConversation.cpp)

set (interface_src 
    interfaceApp/AppInterfaceImpl.cpp
    interfaceApp/MessageEnvelope.pb.cc
    interfaceApp/GroupProtocol.pb.cc
    interfaceTransport/sip/SipTransport.cpp
    interfaceApp/GroupInterfaceImpl.cpp
    interfaceApp/GroupUpdateImpl.cpp
    interfaceApp/SendMessage.cpp
    interfaceApp/ReceiveMessage.cpp
    interfaceApp/QueueHandling.cpp)

if (ANDROID OR JNI_FOUND)
set (interface_src
    ${interface_src}
    interfaceApp/java/JavaNativeImpl.cpp
)
endif()

if (EMSCRIPTEN)
set (interface_src
    ${interface_src}
    interfaceApp/emscripten/JSZina.cpp
)
endif()

set (storage_src
    storage/sqlite/SQLiteStoreConv.cpp
    storage/MessageCapture.cpp
    storage/NameLookup.cpp
    storage/sqlite/VectorClockPersitence.cpp
    storage/sqlite/GroupData.cpp
    storage/sqlite/GroupWaitForAck.cpp
    storage/sqlite/InternalMessageQueues.cpp)

set (key_mngmnt_src
    keymanagment/PreKeys.cpp
)

set (provisioning_src
    provisioning/ScProvisioning.cpp
)

set (dataretention_src
    dataRetention/ScDataRetention.cpp
)

set (util_src
    util/cJSON.c
    util/b64helper.cpp
    util/UUID.cpp
    logging/Logger.cpp
    logging/ZinaLogging.cpp
    util/Utilities.cpp)

set (app_repo_src
    appRepository/AppRepository.cpp
)

set (vector_src
        vectorclock/VectorHelper.cpp
)

if (STANDALONE)
    set (external_src
        ${ZRTP_BASE_DIR}/common/osSpecifics.c)
endif()

add_subdirectory(attachments)

set (zina_src
    ${protocol_src}
    ${storage_src}
    ${interface_src}
    ${key_mngmnt_src}
    ${provisioning_src}
    ${dataretention_src}
    ${util_src}
    ${external_src}
    ${app_repo_src}
    ${vector_src}
)

include(FindDoxygen)
if (DOXYGEN_FOUND)
    if (DOXYGEN_DOT_FOUND)
        set (HAVE_DOT YES)
    else()
        set (HAVE_DOT NO)
    endif()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/ZinaDoxy.in ${CMAKE_CURRENT_SOURCE_DIR}/doc/ZinaDoxy @ONLY)
    add_custom_target(doc
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/ZinaDoxy
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc
            COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()

if (NOT ANDROID)
    add_library(${zinaLibName} ${LIBRARY_BUILD_TYPE} ${zina_src} $<TARGET_OBJECTS:crypto_lib> $<TARGET_OBJECTS:attachment_lib>)
    set_target_properties(${zinaLibName} PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
    TARGET_INCLUDE_DIRECTORIES(${zinaLibName} PUBLIC ${ZRTP_BASE_DIR} ${ZRTP_BASE_DIR}/zrtp)
    target_link_libraries(${zinaLibName} ${LIBS})

    if (UNITTESTS)
        add_subdirectory(gtest-1.7.0)
        add_subdirectory(unittests)
    endif()
endif()

add_custom_target(protobuf COMMAND ${CMAKE_SOURCE_DIR}/protobuf/android/runndk.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/protobuf/android)

add_custom_target(android ${CMAKE_SOURCE_DIR}/android/runndk.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/android)
add_dependencies(android protobuf)

# Prepare Android's Android.mk and Application.mk files
string(REPLACE ";" " " crypto_src_spc "${crypto_src_global}")
string(REPLACE ";" " " attachment_src_spc "${attchment_src_global}")
if (STANDALONE)
    set (zina_src "${zina_src}" ${CMAKE_SOURCE_DIR}/android/jni/sqlite3/sqlite3.c)
else()
    set (EMBEDDED -DEMBEDDED)
endif()
if (UNITTESTS)
    set (EMBEDDED -DUNITTESTS)
endif()

set(NO_VISIBILITY "-fvisibility=hidden -fvisibility-inlines-hidden")

string(REPLACE ";" " " zina_src_spc "${zina_src}")
set(local_cpp_features "exceptions")

configure_file(${CMAKE_SOURCE_DIR}/android/jni/Android.mk
               ${CMAKE_BINARY_DIR}/android/jni/Android.mk @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/android/jni/Application.mk
               ${CMAKE_BINARY_DIR}/android/jni/Application.mk @ONLY)

