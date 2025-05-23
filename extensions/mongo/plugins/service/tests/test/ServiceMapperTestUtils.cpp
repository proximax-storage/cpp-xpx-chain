/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace test {

	namespace {
		void AssertPaymentInformation(const std::vector<state::PaymentInformation>& payments, const bsoncxx::array::view& dbPayments) {
			ASSERT_EQ(payments.size(), test::GetFieldCount(dbPayments));
			auto i = 0u;
			for (const auto& dbPayment : dbPayments) {
				const auto& payment = payments[i++];
				EXPECT_EQ(payment.Receiver, GetKeyValue(dbPayment, "receiver"));
				EXPECT_EQ(payment.Height.unwrap(), GetUint64(dbPayment, "height"));
				EXPECT_EQ(payment.Amount.unwrap(), GetUint64(dbPayment, "amount"));
			}
		}

		void AssertBillingHistory(const std::vector<state::BillingPeriodDescription>& billingHistory, const bsoncxx::array::view& dbBillingHistory) {
			ASSERT_EQ(billingHistory.size(), test::GetFieldCount(dbBillingHistory));
			auto i = 0u;
			for (const auto& dbDescription : dbBillingHistory) {
				const auto& description = billingHistory[i++];
				EXPECT_EQ(description.Start.unwrap(), GetUint64(dbDescription, "start"));
				EXPECT_EQ(description.End.unwrap(), GetUint64(dbDescription, "end"));
				AssertPaymentInformation(description.Payments, dbDescription["payments"].get_array().value);
			}
		}

		void AssertFiles(const state::FilesMap& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, dbFile["fileHash"].get_binary());
				auto info = files.at(fileHash);
				EXPECT_EQ(info.Size, GetUint64(dbFile, "size"));
			}
		}

		void AssertActiveFilesWithoutDeposit(const std::set<Hash256>& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, dbFile.get_binary());
				EXPECT_EQ(1, files.count(fileHash));
			}
		}

		void AssertInactiveFilesWithoutDeposit(const std::map<Hash256, std::vector<Height>>& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, dbFile["fileHash"].get_binary());
				auto heights = files.at(fileHash);
				auto dbHeights = dbFile["heights"].get_array().value;
				ASSERT_EQ(heights.size(), test::GetFieldCount(dbHeights));
				auto counter = 0u;
				for (const auto& dbHeight : dbHeights)
					EXPECT_EQ(heights[counter++].unwrap(), static_cast<uint64_t>(dbHeight.get_int64().value));
			}
		}

		template<typename T>
		void AssertReplicators(const T& replicators, const bsoncxx::array::view& dbReplicatorsMap) {
			ASSERT_EQ(replicators.size(), test::GetFieldCount(dbReplicatorsMap));

			state::ReplicatorsMap replicatorMap;
			for (const auto& replicator : replicators) {
				replicatorMap.emplace(replicator.first, replicator.second);
			}

			for (const auto& dbReplicator : dbReplicatorsMap) {
				Key key;
				DbBinaryToModelArray(key, dbReplicator["replicator"].get_binary());
				const auto& replicator = replicatorMap.at(key);
				EXPECT_EQ(replicator.Start.unwrap(), GetUint64(dbReplicator, "start"));
				EXPECT_EQ(replicator.End.unwrap(), GetUint64(dbReplicator, "end"));

				AssertActiveFilesWithoutDeposit(replicator.ActiveFilesWithoutDeposit, dbReplicator["activeFilesWithoutDeposit"].get_array().value);
				AssertInactiveFilesWithoutDeposit(replicator.InactiveFilesWithoutDeposit, dbReplicator["inactiveFilesWithoutDeposit"].get_array().value);
			}
		}

		void AssertCoowners(const std::set<Key>& coowners, const bsoncxx::array::view& dbCoowners) {
			ASSERT_EQ(coowners.size(), test::GetFieldCount(dbCoowners));
			for (const auto& dbCoowner : dbCoowners) {
				Key coowner;
				DbBinaryToModelArray(coowner, dbCoowner.get_binary());
				EXPECT_EQ(1, coowners.count(coowner));
			}
		}
	}

	void AssertEqualDriveData(const state::DriveEntry& entry, const Address& address, const bsoncxx::document::view& dbDriveEntry) {
		EXPECT_EQ(21u, test::GetFieldCount(dbDriveEntry));

		EXPECT_EQ(entry.key(), GetKeyValue(dbDriveEntry, "multisig"));
		EXPECT_EQ(address, test::GetAddressValue(dbDriveEntry, "multisigAddress"));
		EXPECT_EQ(entry.start().unwrap(), GetUint64(dbDriveEntry, "start"));
		EXPECT_EQ(entry.end().unwrap(), GetUint64(dbDriveEntry, "end"));
		EXPECT_EQ(entry.state(), static_cast<state::DriveState>(static_cast<uint8_t>(dbDriveEntry["state"].get_int32())));
		EXPECT_EQ(entry.owner(), GetKeyValue(dbDriveEntry, "owner"));
		EXPECT_EQ(entry.rootHash(), GetHashValue(dbDriveEntry, "rootHash"));
		EXPECT_EQ(entry.duration().unwrap(), GetUint64(dbDriveEntry, "duration"));
		EXPECT_EQ(entry.billingPeriod().unwrap(), GetUint64(dbDriveEntry, "billingPeriod"));
		EXPECT_EQ(entry.billingPrice().unwrap(), GetUint64(dbDriveEntry, "billingPrice"));
		EXPECT_EQ(entry.size(), static_cast<uint64_t>(dbDriveEntry["size"].get_int64()));
		EXPECT_EQ(entry.occupiedSpace(), static_cast<uint64_t>(dbDriveEntry["occupiedSpace"].get_int64()));
		EXPECT_EQ(entry.replicas(), static_cast<uint16_t>(dbDriveEntry["replicas"].get_int32()));
		EXPECT_EQ(entry.minReplicators(), static_cast<uint16_t>(dbDriveEntry["minReplicators"].get_int32()));
		EXPECT_EQ(entry.percentApprovers(), static_cast<uint8_t>(dbDriveEntry["percentApprovers"].get_int32()));

		AssertBillingHistory(entry.billingHistory(), dbDriveEntry["billingHistory"].get_array().value);
		AssertFiles(entry.files(), dbDriveEntry["files"].get_array().value);
		AssertReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);
		AssertReplicators(entry.removedReplicators(), dbDriveEntry["removedReplicators"].get_array().value);
		AssertPaymentInformation(entry.uploadPayments(), dbDriveEntry["uploadPayments"].get_array().value);
		AssertCoowners(entry.coowners(), dbDriveEntry["coowners"].get_array().value);
	}

	namespace {
		void AssertFiles(const std::map<Hash256, uint64_t>& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, dbFile["fileHash"].get_binary());
				EXPECT_EQ(files.at(fileHash), GetUint64(dbFile, "fileSize"));
			}
		}
	}

	void AssertEqualDownloadData(const state::DownloadEntry& entry, const Address& address, const bsoncxx::document::view& dbDownloadEntry) {
		ASSERT_EQ(6u, test::GetFieldCount(dbDownloadEntry));

		EXPECT_EQ(entry.OperationToken, GetHashValue(dbDownloadEntry, "operationToken"));
		EXPECT_EQ(entry.DriveKey, GetKeyValue(dbDownloadEntry, "driveKey"));
		EXPECT_EQ(address, GetAddressValue(dbDownloadEntry, "driveAddress"));
		EXPECT_EQ(entry.FileRecipient, GetKeyValue(dbDownloadEntry, "fileRecipient"));
		EXPECT_EQ(entry.Height.unwrap(), GetUint64(dbDownloadEntry, "height"));

		AssertFiles(entry.Files, dbDownloadEntry["files"].get_array().value);
	}
}}
