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

        void AssertFileActions(const std::vector<state::DriveAction>& actions, const bsoncxx::array::view& dbActions) {
			ASSERT_EQ(actions.size(), test::GetFieldCount(dbActions));
			auto i = 0u;
            for (const auto& dbAction : dbActions) {
				const auto& action = actions[i++];
				EXPECT_EQ(action.Type, static_cast<state::DriveActionType>(static_cast<int8_t>(dbAction["type"].get_int32())));
				EXPECT_EQ(action.ActionHeight.unwrap(), GetUint64(dbAction, "height"));
            }
        }

		void AssertFiles(const state::FilesMap& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
                Hash256 fileHash;
                DbBinaryToModelArray(fileHash, dbFile["fileHash"].get_binary());
                auto info = files.at(fileHash);
				EXPECT_EQ(info.Deposit.unwrap(), GetUint64(dbFile, "deposit"));
				EXPECT_EQ(info.Size, GetUint64(dbFile, "size"));

				AssertPaymentInformation(info.Payments, dbFile["payments"].get_array().value);
				AssertFileActions(info.Actions, dbFile["actions"].get_array().value);
			}
		}

        void AssertFilesWithoutDeposit(const std::map<Hash256, uint16_t>& files, const bsoncxx::array::view& dbFiles) {
			ASSERT_EQ(files.size(), test::GetFieldCount(dbFiles));
			for (const auto& dbFile : dbFiles) {
                Hash256 fileHash;
                DbBinaryToModelArray(fileHash, dbFile["fileHash"].get_binary());
				EXPECT_EQ(files.at(fileHash), static_cast<uint16_t>(dbFile["count"].get_int32()));
			}
        }

        void AssertReplicators(const state::ReplicatorsMap& replicators, const bsoncxx::array::view& dbReplicatorsMap) {
            for (const auto& dbReplicator : dbReplicatorsMap) {
                Key key;
                DbBinaryToModelArray(key, dbReplicator["replicator"].get_binary());
                const auto& replicator = replicators.at(key);
				EXPECT_EQ(replicator.Deposit.unwrap(), GetUint64(dbReplicator, "deposit"));
				EXPECT_EQ(replicator.Start.unwrap(), GetUint64(dbReplicator, "start"));
				EXPECT_EQ(replicator.End.unwrap(), GetUint64(dbReplicator, "end"));

				AssertFilesWithoutDeposit(replicator.FilesWithoutDeposit, dbReplicator["filesWithoutDeposit"].get_array().value);
            }
        }
	}

	void AssertEqualDriveData(const state::DriveEntry& entry, const Address& address, const bsoncxx::document::view& dbDriveEntry) {
		EXPECT_EQ(17u, test::GetFieldCount(dbDriveEntry));

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
		EXPECT_EQ(entry.replicas(), static_cast<uint16_t>(dbDriveEntry["replicas"].get_int32()));
		EXPECT_EQ(entry.minReplicators(), static_cast<uint16_t>(dbDriveEntry["minReplicators"].get_int32()));
		EXPECT_EQ(entry.percentApprovers(), static_cast<uint8_t>(dbDriveEntry["percentApprovers"].get_int32()));

		AssertBillingHistory(entry.billingHistory(), dbDriveEntry["billingHistory"].get_array().value);
		AssertFiles(entry.files(), dbDriveEntry["files"].get_array().value);
		AssertReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);
	}
}}
