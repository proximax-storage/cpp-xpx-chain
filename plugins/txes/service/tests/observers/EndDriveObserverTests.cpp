/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <cmath>

namespace catapult { namespace observers {

#define TEST_CLASS EndDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndDrive, config::ImmutableConfiguration::Uninitialized())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::EndDriveNotification<1>;

		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);
		constexpr Height Billing_Start(5);
		constexpr Height Billing_End(20);
		constexpr Amount Drive_Balance(1000);
		constexpr auto Drive_Size = 100;
		constexpr auto Num_Replicators = 10;
		constexpr auto Num_Files = 20;

		auto CreateConfig() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.StorageMosaicId = Storage_Mosaic_Id;
			config.StreamingMosaicId = Streaming_Mosaic_Id;
			return config;
		}

		void AddFiles(state::DriveEntry& entry) {
			for (auto i = 0u; i < Num_Files; ++i) {
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				uint64_t fileSize = (i + 1) * 100;
				entry.files().emplace(fileHash, state::FileInfo{ fileSize });
				for (auto& replicator : entry.replicators())
					replicator.second.ActiveFilesWithoutDeposit.insert(fileHash);
			}
		}

		state::DriveEntry CreateInitialDriveEntry(
				std::map<Key, state::AccountState>& initialAccounts,
				state::DriveState driveState) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setSize(Drive_Size);
			entry.setState(driveState);
			entry.setBillingPeriod(BlockDuration((Billing_End - Billing_Start).unwrap()));
			auto driveAccount = test::CreateAccount(entry.key());
			driveAccount.Balances.credit(Storage_Mosaic_Id, Drive_Balance);
			initialAccounts.emplace(entry.key(), driveAccount);

			entry.billingHistory().push_back(state::BillingPeriodDescription{ Billing_Start, Billing_End, {} });

			for (auto i = 0u; i < Num_Replicators; ++i) {
				auto key = test::GenerateRandomByteArray<Key>();
				entry.replicators().emplace(key, state::ReplicatorInfo{ Billing_Start + Height(i + 1), Height(0), {}, {} });
				initialAccounts.emplace(key, test::CreateAccount(key));
			}

			AddFiles(entry);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				state::DriveEntry& initialEntry,
				std::map<Key, state::AccountState>& expectedAccounts,
				const Height& billingEnd) {
			state::DriveEntry entry(initialEntry);
			entry.billingHistory().back().End = billingEnd;
			entry.setState(state::DriveState::Finished);

			auto remainder = Drive_Balance.unwrap();
			auto driveBalance = std::floor(double(remainder));
			auto firstReplicatorStart = (billingEnd - Billing_Start).unwrap() - 1;
			auto lastReplicatorStart = firstReplicatorStart - (Num_Replicators - 1);
			auto sumTime = (firstReplicatorStart + lastReplicatorStart) * Num_Replicators / 2;

			auto& replicators = entry.replicators();
			auto lastReplicatorIter = --replicators.end();
			for (auto replicatorIter = replicators.begin(); replicatorIter != replicators.end(); ++replicatorIter) {
				auto replicatorKey = replicatorIter->first;
				auto replicatorReward = (replicatorIter == lastReplicatorIter) ? Amount(remainder) :
					Amount(driveBalance * (billingEnd - replicatorIter->second.Start).unwrap() / sumTime);
				remainder -= replicatorReward.unwrap();
				expectedAccounts.at(entry.key()).Balances.debit(Storage_Mosaic_Id, replicatorReward, billingEnd);
				auto& replicatorAccount = expectedAccounts.at(replicatorKey);
				replicatorAccount.Balances.credit(Storage_Mosaic_Id, replicatorReward + Amount(Drive_Size), billingEnd);
				entry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{ replicatorKey, replicatorReward, billingEnd });

				auto r = 0u;
				for (const auto& filePair : entry.files()) {
					replicatorIter->second.ActiveFilesWithoutDeposit.erase(filePair.first);
					if (r++ % 2) {
						replicatorIter->second.AddInactiveUndepositedFile(filePair.first, billingEnd);
					} else {
						initialEntry.replicators().at(replicatorIter->first).ActiveFilesWithoutDeposit.erase(filePair.first);
						replicatorAccount.Balances.credit(Streaming_Mosaic_Id, Amount(filePair.second.Size), billingEnd);
					}
				}
			}

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::map<Key, state::AccountState> InitialAccounts;
			std::map<Key, state::AccountState> ExpectedAccounts;
		};

		void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
			// Arrange:
			ObserverTestContext context(mode, currentHeight);
			Notification notification(values.InitialDriveEntry.key(), test::GenerateRandomByteArray<Key>());
			auto config = CreateConfig();
			auto pObserver = CreateEndDriveObserver(config);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			driveCache.addBillingDrive(values.InitialDriveEntry.key(), Billing_End);
			for (const auto& initialAccount : values.InitialAccounts) {
				accountCache.addAccount(initialAccount.second);
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = driveIter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);

			for (const auto& expectedAccount : values.ExpectedAccounts) {
				auto accountIter = accountCache.find(expectedAccount.second.Address);
				const auto& actualAccount = accountIter.get();
				test::AssertAccount(expectedAccount.second, actualAccount);
			}
		}
	}

	TEST(TEST_CLASS, EndDrive_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::Pending);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End);

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Commit_DriveEndedBeforeBillingPeriodEnd) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End - Height(1));

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End - Height(1));
	}

	TEST(TEST_CLASS, EndDrive_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::Pending);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End);

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Rollback_DriveEndedBeforeBillingPeriodEnd) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End - Height(1));

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End - Height(1));
	}
}}
