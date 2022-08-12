/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/StorageNotifications.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DataModificationCancelValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModificationCancel,)

    namespace {
        using Notification = model::DataModificationCancelNotification<1>;

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry,
                const Key& owner,
                const std::vector<state::ActiveDataModification>& activeDataModification) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
            Notification notification(driveEntry.key(), owner, activeDataModification.back().Id);
            auto pValidator = CreateDataModificationCancelValidator();

            // Act:
            auto result = test::ValidateNotification(
                *pValidator,
                notification,
                cache,
                config::BlockchainConfiguration::Uninitialized(),
                Current_Height
            );

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenIsNotOwner) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.setOwner(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
            Failure_Storage_Is_Not_Owner,
            entry,
            test::GenerateRandomByteArray<Key>(),
            {
                {
                    test::GenerateRandomByteArray<Hash256>(),
                    test::GenerateRandomByteArray<Key>(),
                    test::GenerateRandomByteArray<Hash256>(),
                    test::Random()
                }
            }
        );
    }

    TEST(TEST_CLASS, FailureWhenNoActiveDataModification) {
        // Arrange:
        auto owner = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.setOwner(owner);

        // Assert:
        AssertValidationResult(
            Failure_Storage_No_Active_Data_Modifications,
            entry,
            owner,
            {
                {
                    test::GenerateRandomByteArray<Hash256>(),
                    test::GenerateRandomByteArray<Key>(),
                    test::GenerateRandomByteArray<Hash256>(),
                    test::Random()
                }
            }
        );
    }

    TEST(TEST_CLASS, FailureWhenDataModificationNotFound) {
        // Arrange:
        auto owner = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.setOwner(owner);
        entry.activeDataModifications().emplace_back(
            state::ActiveDataModification{
                test::GenerateRandomByteArray<Hash256>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                test::Random()
            }
        );

        // Assert:
        AssertValidationResult(
            Failure_Storage_Data_Modification_Not_Found,
            entry,
            owner,
            {
                {
                test::GenerateRandomByteArray<Hash256>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(), test::Random()
                }
            }
        );
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto owner = test::GenerateRandomByteArray<Key>();
        auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
        std::vector<Key> ownerPublicKey{test::GenerateRandomByteArray<Key>()};
        std::vector<Hash256> downloadDataCdi{test::GenerateRandomByteArray<Hash256>()};
        std::vector<uint64_t> uploadSize{test::Random()};
        state::BcDriveEntry entry(driveKey);
        entry.setOwner(owner);
        entry.activeDataModifications().emplace_back(
            state::ActiveDataModification{
                test::GenerateRandomByteArray<Hash256>(),
                        ownerPublicKey[0],
                        downloadDataCdi[0],
                        uploadSize[0]
            }
        );
        entry.activeDataModifications().emplace_back(
            state::ActiveDataModification{
                dataModificationId,
                ownerPublicKey[1],
                downloadDataCdi[1],
                uploadSize[1]
            }
        );

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            owner,
            {
                {test::GenerateRandomByteArray<Hash256>(), ownerPublicKey[0], downloadDataCdi[0], uploadSize[0]},
                {dataModificationId,                       ownerPublicKey[1], downloadDataCdi[1], uploadSize[1]}
            }
        );
    }
}}