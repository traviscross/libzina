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
#include "gtest/gtest.h"

#include "../ratchet/state/ZinaConversation.h"
#include "../storage/sqlite/SQLiteStoreConv.h"
#include "../ratchet/crypto/EcCurve.h"
#include "../logging/ZinaLogging.h"

using namespace zina;
using namespace std;

static std::string aliceName("alice@wonderland.org");
static std::string bobName("bob@milkyway.com");

static std::string aliceDev("aliceDevId");
static std::string bobDev("BobDevId");

static const uint8_t keyInData[] = {0,1,2,3,4,5,6,7,8,9,19,18,17,16,15,14,13,12,11,10,20,21,22,23,24,25,26,27,28,20,31,30};

class StoreTestFixture: public ::testing::Test {
public:
    StoreTestFixture( ) {
        // initialization code here
    }

    void SetUp() {
        // code here will execute just before the test ensues
        LOGGER_INSTANCE setLogLevel(ERROR);
        store = SQLiteStoreConv::getStore();
        store->setKey(std::string((const char*)keyInData, 32));
        store->openStore(std::string());
    }

    void TearDown( ) {
        // code here will be called just after the test completes
        // ok to through exceptions from here if need be
        SQLiteStoreConv::closeStore();
    }

    ~StoreTestFixture( )  {
        // cleanup any pending stuff, but no exceptions allowed
        LOGGER_INSTANCE setLogLevel(VERBOSE);
    }

    // put in any custom data members that you need
    SQLiteStoreConv* store;
};

TEST_F(StoreTestFixture, BasicEmpty)
{

    // localUser, remote user, remote dev id
    ZinaConversation conv(aliceName, bobName, bobDev);
    conv.storeConversation();
    ASSERT_FALSE(SQL_FAIL(store->getSqlCode())) << store->getLastError();    

    auto conv1 = ZinaConversation::loadConversation(aliceName, bobName, bobDev);
    ASSERT_TRUE(conv1 != NULL);
    ASSERT_TRUE(conv1->getRK().empty());
}

TEST_F(StoreTestFixture, TestDHR)
{
    // localUser, remote user, remote dev id
    ZinaConversation conv(aliceName,   bobName,   bobDev);
    conv.setRatchetFlag(true);

    Ec255PublicKey* pubKey = new Ec255PublicKey(keyInData);
    conv.setDHRr(pubKey);

    const DhKeyPair* keyPair = EcCurve::generateKeyPair(EcCurveTypes::Curve25519);
    conv.setDHRs(keyPair);

    conv.storeConversation();
    auto conv1 = ZinaConversation::loadConversation(aliceName, bobName, bobDev);
    ASSERT_TRUE(conv1 != NULL);
    ASSERT_TRUE(conv1->getRatchetFlag());

    const DhKeyPair* keyPair1 = conv1->getDHRs();
    ASSERT_TRUE(keyPair1 != NULL);
    ASSERT_TRUE(keyPair->getPublicKey() == keyPair1->getPublicKey());

    const DhPublicKey* pubKey1 = conv1->getDHRr();
    ASSERT_TRUE(pubKey1 != NULL);
    ASSERT_TRUE(*pubKey == *pubKey1);
}

TEST_F(StoreTestFixture, TestDHI)
{
    // localUser, remote user, remote dev id
    ZinaConversation conv(aliceName, bobName, bobDev);
    conv.setRatchetFlag(true);

    Ec255PublicKey* pubKey = new Ec255PublicKey(keyInData);
    conv.setDHIr(pubKey);

    const DhKeyPair* keyPair = EcCurve::generateKeyPair(EcCurveTypes::Curve25519);
    conv.setDHIs(keyPair);

    conv.storeConversation();
    auto conv1 = ZinaConversation::loadConversation(aliceName, bobName, bobDev);
    ASSERT_TRUE(conv1 != NULL);
    ASSERT_TRUE(conv1->getRatchetFlag());

    const DhKeyPair* keyPair1 = conv1->getDHIs();
    ASSERT_TRUE(keyPair1 != NULL);
    ASSERT_TRUE(keyPair->getPublicKey() == keyPair1->getPublicKey());

    const DhPublicKey* pubKey1 = conv1->getDHIr();
    ASSERT_TRUE(pubKey1 != NULL);
    ASSERT_TRUE(*pubKey == *pubKey1);
}

TEST_F(StoreTestFixture, TestA0)
{
    // localUser, remote user, remote dev id
    ZinaConversation conv(aliceName,   bobName,   bobDev);
    conv.setRatchetFlag(true);

    const DhKeyPair* keyPair = EcCurve::generateKeyPair(EcCurveTypes::Curve25519);
    conv.setA0(keyPair);

    conv.storeConversation();
    auto conv1 = ZinaConversation::loadConversation(aliceName, bobName, bobDev);
    ASSERT_TRUE(conv1 != NULL);
    ASSERT_TRUE(conv1->getRatchetFlag());

    const DhKeyPair* keyPair1 = conv1->getA0();
    ASSERT_TRUE(keyPair1 != NULL);
    ASSERT_TRUE(keyPair->getPublicKey() == keyPair1->getPublicKey());
}

TEST_F(StoreTestFixture, SimpleFields)
{
    string RK("RootKey");
    string CKs("ChainKeyS 1");
    string CKr("ChainKeyR 1");

    // localUser, remote user, remote dev id
    ZinaConversation conv(aliceName,   bobName,   bobDev);
    conv.setRK(RK);
    conv.setCKr(CKr);
    conv.setCKs(CKs);

    conv.setNr(3);
    conv.setNs(7);
    conv.setPNs(11);
    conv.setPreKeyId(13);
    string tst("test");
    conv.setDeviceName(tst);

    conv.storeConversation();
    auto conv1 = ZinaConversation::loadConversation(aliceName, bobName, bobDev);

    ASSERT_EQ(RK, conv1->getRK());
    ASSERT_EQ(CKr, conv1->getCKr());
    ASSERT_EQ(CKs, conv1->getCKs());

    ASSERT_EQ(3, conv1->getNr());
    ASSERT_EQ(7, conv1->getNs());
    ASSERT_EQ(11, conv1->getPNs());
    ASSERT_EQ(13, conv1->getPreKeyId());
    ASSERT_TRUE(tst == conv1->getDeviceName());
}