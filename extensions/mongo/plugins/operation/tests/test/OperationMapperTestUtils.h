/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"

namespace catapult { namespace state { struct OperationEntry; } }

namespace catapult { namespace test {

	/// Verifies that \a dbOperationEntry is equivalent to model operation \a entry and \a address.
	void AssertEqualMongoOperationData(const state::OperationEntry& entry, const Address& address, const bsoncxx::document::view& dbOperationEntry);

	/// Verifies that mosaics in \a transaction are equivalent to mosaics in \a dbTransaction.
	template<typename TTransaction>
	void AssertEqualMosaics(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
		const auto& mosaicArray = dbTransaction["mosaics"].get_array().value;
		ASSERT_EQ(transaction.MosaicCount, test::GetFieldCount(mosaicArray));
		auto pMosaic = transaction.MosaicsPtr();
		for (const auto& dbMosaic : mosaicArray) {
			auto mosaicDoc = dbMosaic.get_document().view();
			EXPECT_EQ(pMosaic->MosaicId, test::GetUnresolveMosaicId(mosaicDoc, "id"));
			EXPECT_EQ(pMosaic->Amount.unwrap(), test::GetUint64(mosaicDoc, "amount"));
			pMosaic++;
		}
	}
}}
