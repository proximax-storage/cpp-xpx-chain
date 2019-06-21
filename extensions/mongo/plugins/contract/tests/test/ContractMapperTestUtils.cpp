/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		void AssertKeySet(const utils::SortedKeySet& keySet, const bsoncxx::document::view& dbKeySet) {
			ASSERT_EQ(keySet.size(), test::GetFieldCount(dbKeySet));

			for (auto dbIter = dbKeySet.cbegin(); dbKeySet.cend() != dbIter; ++dbIter) {
				Key key;
				mongo::mappers::DbBinaryToModelArray(key, dbIter->get_binary());
				EXPECT_TRUE(keySet.cend() != keySet.find(key)) << "for public key " << key;
			}
		}
	}

	void AssertEqualContractData(const state::ContractEntry& entry, const Address& address, const bsoncxx::document::view& dbContract) {
		EXPECT_EQ(address, test::GetAddressValue(dbContract, "multisigAddress"));

		EXPECT_EQ(entry.key(), GetKeyValue(dbContract, "multisig"));
		EXPECT_EQ(entry.start().unwrap(), GetUint64(dbContract, "start"));
		EXPECT_EQ(entry.duration().unwrap(), GetUint64(dbContract, "duration"));
		EXPECT_EQ(entry.hash(), GetHashValue(dbContract, "hash"));

		auto dbHashes = dbContract["hashes"].get_array().value;
		auto hashesIt = entry.hashes().begin();
		ASSERT_EQ(entry.hashes().size(), test::GetFieldCount(dbHashes));
		for (const auto& dbHash : dbHashes) {
			auto snapshot = dbHash.get_document().view();
			Hash256 hash;
			mongo::mappers::DbBinaryToModelArray(hash, snapshot["hash"].get_binary());
			EXPECT_EQ(hashesIt->Hash, hash);
			EXPECT_EQ(hashesIt->HashHeight, mongo::mappers::GetValue64<Height>(snapshot["height"]));
			++hashesIt;
		}

		AssertKeySet(entry.customers(), dbContract["customers"].get_array().value);
		AssertKeySet(entry.executors(), dbContract["executors"].get_array().value);
		AssertKeySet(entry.verifiers(), dbContract["verifiers"].get_array().value);
	}
}}
