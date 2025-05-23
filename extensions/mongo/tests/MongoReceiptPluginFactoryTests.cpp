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

#include "mongo/src/MongoReceiptPluginFactory.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/test/core/mocks/MockReceipt.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo {

#define TEST_CLASS MongoReceiptPluginFactoryTests

	namespace {
		constexpr auto Mock_Receipt_Type = static_cast<model::ReceiptType>(0xFFFF);

		void Stream(bsoncxx::builder::stream::document& builder, const mocks::MockReceipt& receipt) {
			builder << "version0" << static_cast<int32_t>(receipt.Version);
		}

		static auto CreatePlugin() {
			return MongoReceiptPluginFactory::Create<mocks::MockReceipt>(mocks::MockReceipt::Receipt_Type, Stream);
		}
	}

	// region basic

	TEST(TEST_CLASS, CanCreatePlugin) {
		// Act:
		auto pPlugin = CreatePlugin();

		// Assert:
		EXPECT_EQ(Mock_Receipt_Type, pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamReceipt) {
		// Arrange:
		auto pPlugin = CreatePlugin();
		bsoncxx::builder::stream::document builder;

		mocks::MockReceipt receipt;
		receipt.Version = 0x57;

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		EXPECT_EQ(0x57u, test::GetUint32(view, "version0"));
	}

	// endregion

	// region specific receipts

	TEST(TEST_CLASS, CreateBalanceTransferReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateBalanceTransferReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamBalanceTransferReceipt) {
		// Arrange:
		auto pPlugin = CreateBalanceTransferReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		model::BalanceTransferReceipt receipt(
				model::ReceiptType(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Address>(),
				MosaicId(234),
				Amount(345));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(4u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.Sender, test::GetKeyValue(view, "sender"));
		EXPECT_EQ(receipt.Recipient, test::GetAddressValue(view, "recipient"));
		EXPECT_EQ(receipt.MosaicId, MosaicId(test::GetUint64(view, "mosaicId")));
		EXPECT_EQ(receipt.Amount, Amount(test::GetUint64(view, "amount")));
	}

	TEST(TEST_CLASS, CreateBalanceChangeReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateBalanceChangeReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamBalanceChangeReceipt) {
		// Arrange:
		auto pPlugin = CreateBalanceChangeReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		model::BalanceChangeReceipt receipt(model::ReceiptType(), test::GenerateRandomByteArray<Key>(), MosaicId(234), Amount(345));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(3u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.Account, test::GetKeyValue(view, "account"));
		EXPECT_EQ(receipt.MosaicId, MosaicId(test::GetUint64(view, "mosaicId")));
		EXPECT_EQ(receipt.Amount, Amount(test::GetUint64(view, "amount")));
	}

	TEST(TEST_CLASS, CreateInflationReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateInflationReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamInflationReceipt) {
		// Arrange:
		auto pPlugin = CreateInflationReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		model::InflationReceipt receipt(model::ReceiptType(), MosaicId(234), Amount(345));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(2u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.MosaicId, MosaicId(test::GetUint64(view, "mosaicId")));
		EXPECT_EQ(receipt.Amount, Amount(test::GetUint64(view, "amount")));
	}

	TEST(TEST_CLASS, CanStreamOfferCreationReceipt) {
		// Arrange:
		auto pPlugin = CreateOfferCreationReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		model::OfferCreationReceipt receipt(
			model::ReceiptType(), 
			test::GenerateRandomByteArray<Key>(), 
			std::pair<MosaicId, MosaicId>(234, 345),
			Amount(500), Amount(250));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(5u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.Sender, test::GetKeyValue(view, "sender"));
		EXPECT_EQ(receipt.MosaicsPair.first, MosaicId(test::GetUint64(view, "mosaicIdGive")));
		EXPECT_EQ(receipt.MosaicsPair.second, MosaicId(test::GetUint64(view, "mosaicIdGet")));
		EXPECT_EQ(receipt.AmountGive, Amount(test::GetUint64(view, "mosaicAmountGive")));
		EXPECT_EQ(receipt.AmountGet, Amount(test::GetUint64(view, "mosaicAmountGet")));
	}

	TEST(TEST_CLASS, CreateOfferCreationReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateOfferCreationReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamOfferExchangeReceipt) {
		// Arrange:
		auto pPlugin = CreateOfferExchangeReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		auto pReceipt = CreateOfferExchangeReceipt(
			model::ReceiptType(), 
			test::GenerateRandomByteArray<Key>(), 
			std::pair<MosaicId, MosaicId>(234, 345),
			{model::ExchangeDetail{test::GenerateRandomByteArray<Address>(), std::pair<MosaicId, MosaicId>(345, 234), Amount(350), Amount(700)}});

		// Act:
		pPlugin->streamReceipt(builder, *pReceipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(4u, test::GetFieldCount(view));

		EXPECT_EQ(pReceipt->Sender, test::GetKeyValue(view, "sender"));
		EXPECT_EQ(pReceipt->MosaicsPair.first, MosaicId(test::GetUint64(view, "mosaicIdGive")));
		EXPECT_EQ(pReceipt->MosaicsPair.second, MosaicId(test::GetUint64(view, "mosaicIdGet")));
		auto details = view["exchangeDetails"].get_array().value;
		ASSERT_EQ(pReceipt->ExchangeDetailCount, test::GetFieldCount(details));
		auto pDetail = reinterpret_cast<const model::ExchangeDetail*>(pReceipt.get() + 1);
		for (auto i = 0u; i < pReceipt->ExchangeDetailCount; ++i, ++pDetail) {
			EXPECT_EQ(pDetail->Recipient, test::GetAddressValue(details[i], "recipient"));
			EXPECT_EQ(pDetail->MosaicsPair.first, MosaicId(test::GetUint64(details[i], "mosaicIdGive")));
			EXPECT_EQ(pDetail->MosaicsPair.second, MosaicId(test::GetUint64(details[i], "mosaicIdGet")));
			EXPECT_EQ(pDetail->AmountGive, Amount(test::GetUint64(details[i], "mosaicAmountGive")));
			EXPECT_EQ(pDetail->AmountGet, Amount(test::GetUint64(details[i], "mosaicAmountGet")));
		}
	}

	TEST(TEST_CLASS, CreateOfferExchangeReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateOfferExchangeReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}

	TEST(TEST_CLASS, CanStreamOfferRemovalReceipt) {
		// Arrange:
		auto pPlugin = CreateOfferRemovalReceiptMongoPlugin(model::ReceiptType());
		bsoncxx::builder::stream::document builder;

		model::OfferRemovalReceipt receipt(
			model::ReceiptType(), 
			test::GenerateRandomByteArray<Key>(), 
			std::pair<MosaicId, MosaicId>(234, 345),
			Amount(350));

		// Act:
		pPlugin->streamReceipt(builder, receipt);
		auto dbReceipt = builder << bsoncxx::builder::stream::finalize;

		// Assert:
		auto view = dbReceipt.view();
		EXPECT_EQ(4u, test::GetFieldCount(view));

		EXPECT_EQ(receipt.Sender, test::GetKeyValue(view, "sender"));
		EXPECT_EQ(receipt.MosaicsPair.first, MosaicId(test::GetUint64(view, "mosaicIdGive")));
		EXPECT_EQ(receipt.MosaicsPair.second, MosaicId(test::GetUint64(view, "mosaicIdGet")));
		EXPECT_EQ(receipt.AmountGiveReturned, Amount(test::GetUint64(view, "mosaicAmountGiveReturned")));
	}

	TEST(TEST_CLASS, CreateOfferRemovalReceiptMongoPluginRespectsSuppliedType) {
		// Act:
		auto pPlugin = CreateOfferRemovalReceiptMongoPlugin(static_cast<model::ReceiptType>(1234));

		// Assert:
		EXPECT_EQ(static_cast<model::ReceiptType>(1234), pPlugin->type());
	}
	// endregion
}}
