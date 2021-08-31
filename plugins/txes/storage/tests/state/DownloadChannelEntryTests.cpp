/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DownloadChannelEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadChannelEntryTests

    TEST(TEST_CLASS, DownloadChannelEntry) {
        // Act:
        auto id = test::GenerateRandomByteArray<Hash256>();
        auto entry = DownloadChannelEntry(id);

        // Assert:
        EXPECT_EQ(id, entry.id());
    }

    TEST(TEST_CLASS, CanAccessConsumer) {
        // Arrange:
        auto consumer = test::GenerateRandomByteArray<Key>();
        auto entry = DownloadChannelEntry(Hash256());

        // Sanity:
        ASSERT_EQ(Key(), entry.consumer());

        // Act:
        entry.setConsumer(consumer);

        // Assert:
        EXPECT_EQ(consumer, entry.consumer());
    }

    TEST(TEST_CLASS, CanAccessDrive) {
        // Arrange:
        auto drive = test::GenerateRandomByteArray<Key>();
        auto entry = DownloadChannelEntry(Hash256());

        // Sanity:
        ASSERT_EQ(Key(), entry.drive());

        // Act:
        entry.setDrive(drive);

        // Assert:
        EXPECT_EQ(drive, entry.drive());
    }

    TEST(TEST_CLASS, CanAccessTransactionFee) {
        // Arrange:
        auto transactionFee = Amount(150);
        auto entry = DownloadChannelEntry(Hash256());

        // Sanity:
        ASSERT_EQ(Amount(0), entry.transactionFee());

        // Act:
        entry.setTransactionFee(transactionFee);

        // Assert:
        EXPECT_EQ(transactionFee, entry.transactionFee());
    }

    TEST(TEST_CLASS, CanAccessStorageUnits) {
        // Arrange:
        auto storageUnits = Amount(50);
        auto entry = DownloadChannelEntry(Hash256());

        // Sanity:
        ASSERT_EQ(Amount(0), entry.storageUnits());

        // Act:
        entry.setStorageUnits(storageUnits);

        // Assert:
        EXPECT_EQ(storageUnits, entry.storageUnits());
    }

}}