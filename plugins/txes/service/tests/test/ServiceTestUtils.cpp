/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceTestUtils.h"
#include "catapult/model/Address.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		void GeneratePayments(uint16_t count, std::vector<state::PaymentInformation>& payments) {
			payments.reserve(count);
			for (auto i = 0u; i < count; ++i) {
				payments.emplace_back(state::PaymentInformation{test::GenerateRandomByteArray<Key>(), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Height>()});
			}
		}

		state::FileInfo GenerateFileInfo() {
			state::FileInfo fileInfo;
			fileInfo.Size = test::Random();

			return fileInfo;
		}

		state::ReplicatorInfo GenerateReplicatorInfo(uint8_t activeFilesWithoutDepositCount, uint8_t inactiveFilesWithoutDepositCount, uint8_t heightCount) {
			state::ReplicatorInfo replicatorInfo;
			replicatorInfo.Start = test::GenerateRandomValue<Height>();
			replicatorInfo.End = test::GenerateRandomValue<Height>();
			for (auto i = 0u; i < activeFilesWithoutDepositCount; ++i) {
				replicatorInfo.ActiveFilesWithoutDeposit.emplace(test::GenerateRandomByteArray<Hash256>());
			}
			for (auto i = 0u; i < inactiveFilesWithoutDepositCount; ++i) {
				std::vector<Height> heights;
				heights.reserve(heightCount);
				for (auto k = 0u; k < heightCount; ++k)
					heights.push_back(test::GenerateRandomValue<Height>());
				replicatorInfo.InactiveFilesWithoutDeposit.emplace(test::GenerateRandomByteArray<Hash256>(), heights);
			}

			return replicatorInfo;
		}

		void AssertEqualPayments(const std::vector<state::PaymentInformation>& expectedPayments, const std::vector<state::PaymentInformation>& payments) {
			ASSERT_EQ(expectedPayments.size(), payments.size());
			for (auto i = 0u; i < payments.size(); ++i) {
				const auto &expectedPayment = expectedPayments[i];
				const auto &payment = payments[i];
				EXPECT_EQ(expectedPayment.Receiver, payment.Receiver);
				EXPECT_EQ(expectedPayment.Amount, payment.Amount);
				EXPECT_EQ(expectedPayment.Height, payment.Height);
			}
		}
	}

	state::DriveEntry CreateDriveEntry(
			Key key,
			uint16_t billingHistoryCount,
			uint16_t drivePaymentCount,
			uint16_t fileCount,
			uint16_t replicatorCount,
			uint16_t activeFilesWithoutDepositCount,
			uint16_t inactiveFilesWithoutDepositCount,
			uint16_t heightCount,
			uint16_t removedReplicatorCount,
			uint16_t uploadPaymentCount) {
		state::DriveEntry entry(key);
		entry.setStart(test::GenerateRandomValue<Height>());
		entry.setEnd(test::GenerateRandomValue<Height>());
		entry.setState(static_cast<state::DriveState>(test::RandomByte()));
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(test::GenerateRandomByteArray<Hash256>());
		entry.setDuration(test::GenerateRandomValue<BlockDuration>());
		entry.setBillingPeriod(test::GenerateRandomValue<BlockDuration>());
		entry.setBillingPrice(test::GenerateRandomValue<Amount>());

		entry.billingHistory().reserve(billingHistoryCount);
		for (auto i = 0u; i < billingHistoryCount; ++i) {
			entry.billingHistory().emplace_back(state::BillingPeriodDescription{test::GenerateRandomValue<Height>(), test::GenerateRandomValue<Height>(), {}});
			GeneratePayments(drivePaymentCount, entry.billingHistory()[i].Payments);
		}

		entry.setSize(test::Random());
		entry.setOccupiedSpace(test::Random());
		entry.setReplicas(test::Random16());
		entry.setMinReplicators(test::Random16());
		entry.setPercentApprovers(test::RandomByte());

		for (auto i = 0u; i < fileCount; ++i) {
			entry.files().emplace(test::GenerateRandomByteArray<Hash256>(), GenerateFileInfo());
		}

		for (auto i = 0u; i < replicatorCount; ++i) {
			entry.replicators().emplace(test::GenerateRandomByteArray<Key>(),
			    GenerateReplicatorInfo(activeFilesWithoutDepositCount, inactiveFilesWithoutDepositCount, heightCount));
		}

		entry.removedReplicators().reserve(removedReplicatorCount);
		for (auto i = 0u; i < removedReplicatorCount; ++i) {
			entry.removedReplicators().push_back({test::GenerateRandomByteArray<Key>(),
			    GenerateReplicatorInfo(activeFilesWithoutDepositCount, inactiveFilesWithoutDepositCount, heightCount)});
		}

		entry.uploadPayments().reserve(uploadPaymentCount);
		for (auto i = 0u; i < uploadPaymentCount; ++i) {
			entry.uploadPayments().push_back({test::GenerateRandomByteArray<Key>(), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Height>()});
		}

		return entry;
	}

	state::AccountState CreateAccount(const Key& owner, model::NetworkIdentifier networkIdentifier) {
		auto address = model::PublicKeyToAddress(owner, networkIdentifier);
		state::AccountState account(address, Height(1));
		account.PublicKey = owner;
		account.PublicKeyHeight = Height(1);
		return account;
	}

	void AssertAccount(const state::AccountState& expected, const state::AccountState& actual) {
		ASSERT_EQ(expected.Balances.size(), actual.Balances.size());
		for (auto iter = expected.Balances.begin(); iter != expected.Balances.end(); ++iter)
			EXPECT_EQ(iter->second, actual.Balances.get(iter->first));
	}

	state::MultisigEntry CreateMultisigEntry(const state::DriveEntry& driveEntry) {
		state::MultisigEntry multisigEntry(driveEntry.key());

		for (const auto& replicatorPair : driveEntry.replicators()) {
			multisigEntry.cosignatories().insert(replicatorPair.first);
		}
		float cosignatoryCount = driveEntry.replicators().size();
		uint8_t minCosignatory = ceil(cosignatoryCount * driveEntry.percentApprovers() / 100);
		multisigEntry.setMinApproval(minCosignatory);
		multisigEntry.setMinRemoval(minCosignatory);

		return multisigEntry;
	}

	void AssertMultisig(const cache::MultisigCacheDelta& cache, const state::MultisigEntry& expected) {
		if (expected.cosignatories().size()) {
			auto multisigIter = cache.find(expected.key());
			const auto& actualMultisigEntry = multisigIter.get();
			ASSERT_EQ(expected.cosignatories().size(), actualMultisigEntry.cosignatories().size());
			for (const auto& key : expected.cosignatories())
				EXPECT_EQ(1, actualMultisigEntry.cosignatories().count(key));
			EXPECT_EQ(expected.minApproval(), actualMultisigEntry.minApproval());
			EXPECT_EQ(expected.minRemoval(), actualMultisigEntry.minRemoval());
		} else {
			EXPECT_FALSE(cache.contains(expected.key()));
		}
	}

	template<typename T>
	void AssertEqualReplicators(const T& expectedReplicators, const T& replicators) {
		ASSERT_EQ(expectedReplicators.size(), replicators.size());
		auto expectedReplicatorIter = expectedReplicators.begin();
		auto replicatorIter = replicators.begin();
		for (; replicatorIter != replicators.end(); ++replicatorIter, ++expectedReplicatorIter) {
			EXPECT_EQ(expectedReplicatorIter->first, replicatorIter->first);
			const auto& expectedReplicator = expectedReplicatorIter->second;
			const auto& replicator = replicatorIter->second;
			EXPECT_EQ(expectedReplicator.Start, replicator.Start);
			EXPECT_EQ(expectedReplicator.End, replicator.End);

			const auto& expectedActiveFilesWithoutDeposit = expectedReplicator.ActiveFilesWithoutDeposit;
			const auto& activeFilesWithoutDeposit = replicator.ActiveFilesWithoutDeposit;
			ASSERT_EQ(expectedActiveFilesWithoutDeposit.size(), activeFilesWithoutDeposit.size());
			for (const auto& fileHash : activeFilesWithoutDeposit) {
				EXPECT_EQ(1, expectedActiveFilesWithoutDeposit.count(fileHash));
			}

			const auto& expectedInactiveFilesWithoutDeposit = expectedReplicator.InactiveFilesWithoutDeposit;
			const auto& inactiveFilesWithoutDeposit = replicator.InactiveFilesWithoutDeposit;
			ASSERT_EQ(expectedInactiveFilesWithoutDeposit.size(), inactiveFilesWithoutDeposit.size());
			for (const auto& pair : inactiveFilesWithoutDeposit) {
				const auto& expectedHeights = expectedInactiveFilesWithoutDeposit.at(pair.first);
				const auto& heights = pair.second;
				ASSERT_EQ(expectedHeights.size(), heights.size());
				for (auto i = 0u; i < heights.size(); ++i)
					EXPECT_EQ(expectedHeights[i], heights[i]);
			}
		}
	}

	void AssertEqualDriveData(const state::DriveEntry& expectedEntry, const state::DriveEntry& entry) {
		EXPECT_EQ(expectedEntry.key(), entry.key());
		EXPECT_EQ(expectedEntry.start(), entry.start());
		EXPECT_EQ(expectedEntry.end(), entry.end());
		EXPECT_EQ(expectedEntry.state(), entry.state());
		EXPECT_EQ(expectedEntry.owner(), entry.owner());
		EXPECT_EQ(expectedEntry.rootHash(), entry.rootHash());
		EXPECT_EQ(expectedEntry.duration(), entry.duration());
		EXPECT_EQ(expectedEntry.billingPeriod(), entry.billingPeriod());
		EXPECT_EQ(expectedEntry.billingPrice(), entry.billingPrice());

		const auto& expectedBillingHistory = expectedEntry.billingHistory();
		const auto& billingHistory = entry.billingHistory();
		ASSERT_EQ(expectedBillingHistory.size(), billingHistory.size());
		for (auto i = 0u; i < billingHistory.size(); ++i) {
			const auto& expectedBillingPeriod = expectedBillingHistory[i];
			const auto& billingPeriod = billingHistory[i];
			EXPECT_EQ(expectedBillingPeriod.Start, billingPeriod.Start);
			EXPECT_EQ(expectedBillingPeriod.End, billingPeriod.End);
			AssertEqualPayments(expectedBillingPeriod.Payments, billingPeriod.Payments);
		}

		EXPECT_EQ(expectedEntry.size(), entry.size());
		EXPECT_EQ(expectedEntry.occupiedSpace(), entry.occupiedSpace());
		EXPECT_EQ(expectedEntry.replicas(), entry.replicas());
		EXPECT_EQ(expectedEntry.minReplicators(), entry.minReplicators());
		EXPECT_EQ(expectedEntry.percentApprovers(), entry.percentApprovers());

		const auto& expectedFiles = expectedEntry.files();
		const auto& files = entry.files();
		ASSERT_EQ(expectedFiles.size(), files.size());
		for (const auto& filePair : files) {
			const auto& expectedFile = expectedFiles.at(filePair.first);
			const auto& file = filePair.second;
			EXPECT_EQ(expectedFile.Size, file.Size);
		}

		AssertEqualReplicators(expectedEntry.replicators(), entry.replicators());
		AssertEqualReplicators(expectedEntry.removedReplicators(), entry.removedReplicators());

		AssertEqualPayments(expectedEntry.uploadPayments(), entry.uploadPayments());
	}

	state::DownloadEntry CreateDownloadEntry(
			Key driveKey,
			uint32_t fileRecipientCount,
			uint16_t downloadCount,
			uint16_t fileCount) {
		state::DownloadEntry entry(driveKey);
		auto& fileRecipients = entry.fileRecipients();
		for (auto i = 0u; i < fileRecipientCount; ++i) {
			auto fileRecipient = test::GenerateRandomByteArray<Key>();
			state::DownloadMap downloads;
			for (auto k = 0u; k < downloadCount; ++k) {
				auto operationToken = test::GenerateRandomByteArray<Hash256>();
				std::set<Hash256> fileHashes;
				for (auto l = 0u; l < fileCount; ++l)
					fileHashes.insert(test::GenerateRandomByteArray<Hash256>());
				downloads.emplace(operationToken, fileHashes);
			}
			fileRecipients.emplace(fileRecipient, downloads);
		}

		return entry;
	}

	void AssertEqualDownloadData(const state::DownloadEntry& entry1, const state::DownloadEntry& entry2) {
		EXPECT_EQ(entry1.driveKey(), entry2.driveKey());
		ASSERT_EQ(entry1.fileRecipients().size(), entry2.fileRecipients().size());
		const auto& fileRecipients1 = entry1.fileRecipients();
		const auto& fileRecipients2 = entry2.fileRecipients();
		for (const auto& fileRecipientPair : fileRecipients1) {
			const auto& downloads1 = fileRecipientPair.second;
			ASSERT_TRUE(fileRecipients2.count(fileRecipientPair.first));
			const auto& downloads2 = fileRecipients2.at(fileRecipientPair.first);
			ASSERT_EQ(downloads1.size(), downloads2.size());
			for (const auto& downloadPair : downloads1) {
				const auto& fileHashes1 = downloadPair.second;
				ASSERT_TRUE(downloads2.count(downloadPair.first));
				const auto& fileHashes2 = downloads2.at(downloadPair.first);
				ASSERT_EQ(fileHashes1.size(), fileHashes2.size());
				for (const auto& fileHash : fileHashes1) {
					ASSERT_TRUE(fileHashes2.count(fileHash));
				}
			}
		}

	}
}}


