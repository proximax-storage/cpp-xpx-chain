/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "catapult/model/StorageNotifications.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS StreamPaymentObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(StreamPayment,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::StreamPaymentNotification<1>;

        constexpr auto Current_Height = Height(10);

        struct BcDriveValues {
            public:
                explicit BcDriveValues(
						const Key& key,
						const state::ActiveDataModifications& modifications,
						const uint64_t& additionalSize,
						const Hash256& streamId)
                    : Drive_Key(key)
                    , Active_Data_Modifications(modifications)
                    , Additional_Size(additionalSize)
					, Stream_Id(streamId)
                {}

            public:
                Key Drive_Key;
                std::vector<state::ActiveDataModification> Active_Data_Modifications;
                uint64_t Additional_Size;
                Hash256 Stream_Id;
        };

        state::BcDriveEntry CreateEntry(const BcDriveValues& values) {
            state::BcDriveEntry entry(values.Drive_Key);
            for (const auto &activeDataModification : values.Active_Data_Modifications) {
                entry.activeDataModifications().emplace_back(activeDataModification);
            }
			for (auto& modification: entry.activeDataModifications()) {
				if (modification.Id == values.Stream_Id) {
					modification.ExpectedUploadSize += values.Additional_Size;
					modification.ActualUploadSize += values.Additional_Size;
					break;
				}
			}
            return entry;
        }

		auto RandomBcDriveValues() {
			auto key = test::GenerateRandomByteArray<Key>();
			auto expectedUploadSize = test::Random();
			auto folderNameBytes = test::GenerateRandomVector(512);
			auto streamId = test::GenerateRandomByteArray<Hash256>();
			state::ActiveDataModifications modifications = {
				state::ActiveDataModification(
						test::GenerateRandomByteArray<Hash256>(),
						test::GenerateRandomByteArray<Key>(),
						1,
						std::string(folderNameBytes.begin(), folderNameBytes.end())),
				state::ActiveDataModification(
						streamId,
						test::GenerateRandomByteArray<Key>(),
						2,
						std::string(folderNameBytes.begin(), folderNameBytes.end()))
			};
			return BcDriveValues(key, modifications, 4, streamId);
		}

        void RunTest(NotifyMode mode, const BcDriveValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
				values.Drive_Key,
                values.Stream_Id,
                values.Additional_Size);
            auto pObserver = CreateStreamPaymentObserver();
        	auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
			auto entry = state::BcDriveEntry(values.Drive_Key);
			auto modifications = values.Active_Data_Modifications;
			entry.activeDataModifications() = modifications;
            bcDriveCache.insert(entry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

             // Assert: check the cache
             auto driveIter = bcDriveCache.find(values.Drive_Key);
             const auto& actualEntry = driveIter.get();
             test::AssertEqualBcDriveData(CreateEntry(values), actualEntry);
        }
    }

    TEST(TEST_CLASS, StreamPayment_Commit) {
        // Arrange:
        auto values = RandomBcDriveValues();

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, StreamPayment_Rollback) {
        // Arrange:
		auto values = RandomBcDriveValues();

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}