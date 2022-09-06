/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/model/LockFundTotalStakedReceipt.h"
#include "src/LockFundReceiptMapper.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "plugins/txes/lock_fund/src/model/LockFundReceiptType.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/test/core/mocks/MockReceipt.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MosaicExpiryReceiptMapperTests

	TEST(TEST_CLASS, CanCreateLockFundReceiptPlugin) {
		// Act:
		auto pPlugin = CreateLockFundReceiptMongoPlugin();

		// Assert:
		EXPECT_EQ(model::Receipt_Type_Total_Staked, pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamTotalStakedReceipt) {
		// Arrange:
		auto pPlugin = CreateLockFundReceiptMongoPlugin();
		bsoncxx::builder::stream::document builder;

		model::TotalStakedReceipt receipt(model::ReceiptType(), Amount(234));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.Amount, Amount(test::GetUint64(view, "amount")));
	}
}}}
