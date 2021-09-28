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

#define TEST_CLASS EndDriveVerificationObserverTests

        DEFINE_COMMON_OBSERVER_TESTS(EndDriveVerification,)

        namespace {
            using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
            using Notification = model::EndDriveVerificationNotification<1>;

            constexpr auto Current_Height = Height(10);
            const auto Verification_Trigger = test::GenerateRandomByteArray<Hash256>();

            state::Verification CreateInitialVerificationEntry() {
                return state::Verification{
                        Verification_Trigger,
                        state::VerificationResults{},
                        state::VerificationState::Pending,
                };
            }

            state::Verification CreateExpectedVerificationEntry(const state::VerificationResults& results) {
                return state::Verification{
                        Verification_Trigger,
                        results,
                        state::VerificationState::Finished,
                };
            }

            struct CacheValues {
            public:
                explicit CacheValues()
                        : InitialVerificationEntry()
                        , ExpectedVerificationEntry()
                {}

            public:
                state::Verification InitialVerificationEntry;
                state::Verification ExpectedVerificationEntry;
            };

            void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
                // Arrange:
                ObserverTestContext context(mode, currentHeight);

                auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
                auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
                auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

                // Populate cache.
                state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
                driveEntry.verifications().emplace_back(values.InitialVerificationEntry);

                std::vector<model::VerificationOpinion> verificationOpinion;
                for (auto& resultPair : values.ExpectedVerificationEntry.Results) {
                    const auto& verifier = resultPair.first;
                    const auto& blsKey = test::GenerateRandomByteArray<BLSPublicKey>();

                    accountStateCache.addAccount(verifier, currentHeight);

                    state::ReplicatorEntry replicatorEntry(verifier);
                    replicatorEntry.setBlsKey(blsKey);
                    replicatorEntry.setCapacity(Amount(1000));
                    replicatorEntry.drives().emplace(driveEntry.key(), state::DriveInfo{Hash256(), false, 0 });
                    replicatorCache.insert(replicatorEntry);

                    driveEntry.replicators().emplace(replicatorEntry.key());

                    std::vector<std::pair<Key, uint8_t>> opinions;
                    for (auto& result : values.ExpectedVerificationEntry.Results) {
                        if (verifier == result.first)
                            continue;

                        opinions.emplace_back(std::pair<Key, uint8_t>{result.first, result.second});
                    }

                    verificationOpinion.emplace_back(model::VerificationOpinion{
                            verifier,
                            test::GenerateRandomByteArray<BLSSignature>(),
                            opinions,
                    });
                }
                bcDriveCache.insert(driveEntry);

                Notification notification(
                        driveEntry.key(),
                        driveEntry.verifications().back().VerificationTrigger,
                        driveEntry.replicators().size(),
                        &(*driveEntry.replicators().begin()),
                        verificationOpinion.size(),
                        &(*verificationOpinion.begin())
                );

                auto pObserver = CreateEndDriveVerificationObserver();

                // Act:
                test::ObserveNotification(*pObserver, notification, context);

                // Assert: check the cache
                auto driveIter = bcDriveCache.find(driveEntry.key());
                const auto& actualDrive = driveIter.tryGet();
                const auto& actualEntry = actualDrive->verifications().back();

                EXPECT_EQ(values.ExpectedVerificationEntry.VerificationTrigger, actualEntry.VerificationTrigger);
                EXPECT_EQ(values.ExpectedVerificationEntry.State, actualEntry.State);
                EXPECT_EQ(values.ExpectedVerificationEntry.Results, actualEntry.Results);
            }
        }

        TEST(TEST_CLASS, EndDriveVerification_Commit) {
            // Arrange:
            CacheValues values;

            values.InitialVerificationEntry = CreateInitialVerificationEntry();

            // TODO add results
            state::VerificationResults results;
            values.ExpectedVerificationEntry = CreateExpectedVerificationEntry(results);

            // Assert
            RunTest(NotifyMode::Commit, values, Current_Height);
        }

        TEST(TEST_CLASS, EndDriveVerification_Rollback) {
            // Arrange:
            CacheValues values;
            auto driveKey = test::GenerateRandomByteArray<Key>();

            values.ExpectedVerificationEntry = CreateExpectedVerificationEntry(state::VerificationResults{});
            values.InitialVerificationEntry = values.ExpectedVerificationEntry;

            // Assert
            EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
        }

    }}