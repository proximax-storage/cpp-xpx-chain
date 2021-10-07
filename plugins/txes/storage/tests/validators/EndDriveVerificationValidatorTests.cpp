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

namespace catapult {
    namespace validators {

#define TEST_CLASS EndDriveVerificationValidatorTests

        DEFINE_COMMON_VALIDATOR_TESTS(EndDriveVerification,)

        namespace {
            using Notification = model::EndDriveVerificationNotification<1>;

            constexpr auto Current_Height = Height(10);

            void AssertValidationResult(
                    ValidationResult expectedResult,
                    const state::BcDriveEntry& driveEntry,
                    const Key& driveKey,
                    const Hash256& trigger,
                    const uint16_t proversCount,
                    const Key* proversPtr,
                    const uint16_t verificationOpinionsCount,
                    const model::VerificationOpinion* verificationOpinionsPtr) {
                // Arrange:
                auto cache = test::BcDriveCacheFactory::Create();
                {
                    auto delta = cache.createDelta();
                    auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                    driveCacheDelta.insert(driveEntry);
                    cache.commit(Current_Height);
                }

                Notification notification(
                        driveKey,
                        trigger,
                        proversCount,
                        proversPtr,
                        verificationOpinionsCount,
                        verificationOpinionsPtr
                );

                auto pValidator = CreateEndDriveVerificationValidator();

                // Act:
                auto result = test::ValidateNotification(*pValidator, notification, cache,
                                                         config::BlockchainConfiguration::Uninitialized(),
                                                         Current_Height);

                // Assert:
                EXPECT_EQ(expectedResult, result);
            }
        }

        TEST(TEST_CLASS, FailureWhenDriveNotFound) {
            // Arrange:
            state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());

            // Assert:
            AssertValidationResult(
                    Failure_Storage_Drive_Not_Found,
                    entry,
                    test::GenerateRandomByteArray<Key>(),
                    test::GenerateRandomByteArray<Hash256>(),
                    0,
                    nullptr,
                    0,
                    nullptr
            );
        }

        TEST(TEST_CLASS, FailureBadVerificationTrigger) {
            // Arrange:
            Key driveKey = test::GenerateRandomByteArray<Key>();
            state::BcDriveEntry entry(driveKey);
            entry.verifications().emplace_back(state::Verification{test::GenerateRandomByteArray<Hash256>()});

            // Assert:
            AssertValidationResult(
                    Failure_Storage_Verification_Bad_Verification_Trigger,
                    entry,
                    driveKey,
                    test::GenerateRandomByteArray<Hash256>(),
                    0,
                    nullptr,
                    0,
                    nullptr
            );
        }

        TEST(TEST_CLASS, FailureWrongNumberOfProvers) {
            // Arrange:
            Key driveKey = test::GenerateRandomByteArray<Key>();
            state::BcDriveEntry entry(driveKey);

            auto trigger = test::GenerateRandomByteArray<Hash256>();
            entry.verifications().emplace_back(state::Verification{trigger});

            // Assert:
            AssertValidationResult(
                    Failure_Storage_Verification_Wrong_Number_Of_Provers,
                    entry,
                    driveKey,
                    trigger,
                    1,
                    nullptr,
                    0,
                    nullptr
            );
        }

        TEST(TEST_CLASS, FailureSomeProversAreIllegal) {
            // Arrange:
            Key driveKey = test::GenerateRandomByteArray<Key>();
            state::BcDriveEntry entry(driveKey);

            auto trigger = test::GenerateRandomByteArray<Hash256>();
            entry.verifications().emplace_back(state::Verification{
                trigger,
                state::VerificationState::Finished,
                state::VerificationResults{{test::GenerateRandomByteArray<Key>(), 1}}
            });

            const Key prover = test::GenerateRandomByteArray<Key>();

            // Assert:
            AssertValidationResult(
                    Failure_Storage_Verification_Some_Provers_Are_Illegal,
                    entry,
                    driveKey,
                    trigger,
                    1,
                    &prover,
                    0,
                    nullptr
            );
        }

        TEST(TEST_CLASS, FailureNotInPending) {
            // Arrange:
            Key driveKey = test::GenerateRandomByteArray<Key>();
            state::BcDriveEntry entry(driveKey);

            auto trigger = test::GenerateRandomByteArray<Hash256>();
            entry.verifications().emplace_back(state::Verification{
                trigger,
                state::VerificationState::Finished,
                state::VerificationResults{},
            });

            // Assert:
            AssertValidationResult(
                    Failure_Storage_Verification_Not_In_Pending,
                    entry,
                    driveKey,
                    trigger,
                    0,
                    nullptr,
                    0,
                    nullptr
            );
        }

        TEST(TEST_CLASS, Success) {
            // Arrange:
            Key driveKey = test::GenerateRandomByteArray<Key>();
            state::BcDriveEntry entry(driveKey);

            auto trigger = test::GenerateRandomByteArray<Hash256>();
            entry.verifications().emplace_back(state::Verification{trigger});

            // Assert:
            AssertValidationResult(
                    ValidationResult::Success,
                    entry,
                    driveKey,
                    trigger,
                    0,
                    nullptr,
                    0,
                    nullptr
            );
        }
    }
}