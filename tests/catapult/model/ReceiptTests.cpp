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

#include "catapult/model/Receipt.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS ReceiptTests

	// region Receipt

	TEST(TEST_CLASS, ReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(uint32_t) // size
				+ sizeof(uint32_t) // version
				+ sizeof(uint16_t); // type

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(Receipt));
		EXPECT_EQ(10u, sizeof(Receipt));
	}

	// endregion

	// region BalanceTransferReceipt

	TEST(TEST_CLASS, BalanceTransferReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ Key_Size // sender
				+ Address_Decoded_Size // receipient
				+ sizeof(MosaicId) // mosaic id
				+ sizeof(Amount); // amount

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(BalanceTransferReceipt));
		EXPECT_EQ(10u + 73, sizeof(BalanceTransferReceipt));
	}

	TEST(TEST_CLASS, CanCreateBalanceTransferReceipt) {
		// Arrange:
		auto sender = test::GenerateRandomByteArray<Key>();
		auto recipient = test::GenerateRandomByteArray<Address>();

		// Act:
		BalanceTransferReceipt receipt(static_cast<ReceiptType>(123), sender, recipient, MosaicId(88), Amount(452));

		// Assert:
		ASSERT_EQ(sizeof(BalanceTransferReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(123), receipt.Type);
		EXPECT_EQ(sender, receipt.Sender);
		EXPECT_EQ(recipient, receipt.Recipient);
		EXPECT_EQ(MosaicId(88), receipt.MosaicId);
		EXPECT_EQ(Amount(452), receipt.Amount);
	}

	// endregion

	// region BalanceChangeReceipt

	TEST(TEST_CLASS, BalanceChangeReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ Key_Size // account
				+ sizeof(MosaicId) // mosaic id
				+ sizeof(Amount); // amount

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(BalanceChangeReceipt));
		EXPECT_EQ(10u + 48, sizeof(BalanceChangeReceipt));
	}

	TEST(TEST_CLASS, CanCreateBalanceChangeReceipt) {
		// Arrange:
		auto account = test::GenerateRandomByteArray<Key>();

		// Act:
		BalanceChangeReceipt receipt(static_cast<ReceiptType>(124), account, MosaicId(88), Amount(452));

		// Assert:
		ASSERT_EQ(sizeof(BalanceChangeReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(124), receipt.Type);
		EXPECT_EQ(account, receipt.Account);
		EXPECT_EQ(MosaicId(88), receipt.MosaicId);
		EXPECT_EQ(Amount(452), receipt.Amount);
	}

	// endregion

	// region InflationReceipt

	TEST(TEST_CLASS, InflationReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ sizeof(MosaicId) // mosaic id
				+ sizeof(Amount); // amount

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(InflationReceipt));
		EXPECT_EQ(10u + 16, sizeof(InflationReceipt));
	}

	TEST(TEST_CLASS, CanCreateInflationReceipt) {
		// Act:
		InflationReceipt receipt(static_cast<ReceiptType>(124), MosaicId(88), Amount(452));

		// Assert:
		ASSERT_EQ(sizeof(InflationReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(124), receipt.Type);
		EXPECT_EQ(MosaicId(88), receipt.MosaicId);
		EXPECT_EQ(Amount(452), receipt.Amount);
	}

	// endregion

	// region ArtifactExpiryReceipt

	TEST(TEST_CLASS, ArtifactExpiryReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ sizeof(uint64_t); // id

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(ArtifactExpiryReceipt<uint64_t>));
		EXPECT_EQ(10u + 8, sizeof(ArtifactExpiryReceipt<uint64_t>));
	}

	TEST(TEST_CLASS, CanCreateArtifactExpiryReceipt) {
		// Act:
		ArtifactExpiryReceipt<uint64_t> receipt(static_cast<ReceiptType>(125), 8899);

		// Assert:
		ASSERT_EQ(sizeof(ArtifactExpiryReceipt<uint64_t>), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(125), receipt.Type);
		EXPECT_EQ(8899u, receipt.ArtifactId);
	}

	// endregion

	// region OfferCreationReceipt

	TEST(TEST_CLASS, OfferCreationReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ Key_Size // sender
				+ sizeof(MosaicId)*2 // mosaic id pair
				+ sizeof(Amount)*2; // amount to give and to get

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(OfferCreationReceipt));
		EXPECT_EQ(10u + 64, sizeof(OfferCreationReceipt));
	}

	TEST(TEST_CLASS, CanCreateOfferCreationReceipt) {
		// Arrange:
		auto sender = test::GenerateRandomByteArray<Key>();

		// Act:
		OfferCreationReceipt receipt(static_cast<ReceiptType>(126), sender, std::pair<MosaicId,MosaicId>(88,8080), Amount(500), Amount(250));

		// Assert:
		ASSERT_EQ(sizeof(OfferCreationReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(126), receipt.Type);
		EXPECT_EQ(MosaicId(88), receipt.MosaicsPair.first);
		EXPECT_EQ(MosaicId(8080), receipt.MosaicsPair.second);
		EXPECT_EQ(Amount(500), receipt.AmountGive);
		EXPECT_EQ(Amount(250), receipt.AmountGet);
	}

	// endregion

	// region OfferExchangeReceipt

	TEST(TEST_CLASS, CanCreateOfferExchangeReceipt) {
		// Arrange:
		auto sender = test::GenerateRandomByteArray<Key>();
		auto recipient = test::GenerateRandomByteArray<Address>();

		// Act:
		auto pReceipt = CreateOfferExchangeReceipt(static_cast<ReceiptType>(127), sender, std::pair<MosaicId,MosaicId>(88,8080), std::vector<ExchangeDetail>{{recipient, std::pair<MosaicId,MosaicId>(8080,88), Amount(350), Amount(700)}});

		// Assert:
		ASSERT_EQ(sizeof(OfferExchangeReceipt) + sizeof(ExchangeDetail), pReceipt->Size);
		EXPECT_EQ(1u, pReceipt->Version);
		EXPECT_EQ(static_cast<ReceiptType>(127), pReceipt->Type);
		EXPECT_EQ(MosaicId(88), pReceipt->MosaicsPair.first);
		EXPECT_EQ(MosaicId(8080), pReceipt->MosaicsPair.second);
		auto pDetail = reinterpret_cast<const ExchangeDetail*>(pReceipt.get() + 1);
		for (auto i = 0u; i < pReceipt->ExchangeDetailCount; ++i, ++pDetail) {
			EXPECT_EQ(recipient, pDetail->Recipient);
			EXPECT_EQ(MosaicId(8080), pDetail->MosaicsPair.first);
			EXPECT_EQ(MosaicId(88), pDetail->MosaicsPair.second);
			EXPECT_EQ(Amount(350), pDetail->AmountGive);
			EXPECT_EQ(Amount(700), pDetail->AmountGet);
		}
	}

	// endregion

	// region OfferRemovalReceipt

	TEST(TEST_CLASS, OfferRemovalReceiptHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(Receipt) // base
				+ Key_Size // sender
				+ sizeof(MosaicId)*2 // mosaic id pair
				+ sizeof(Amount); // amount to give that has been returned

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(OfferRemovalReceipt));
		EXPECT_EQ(10u + 56, sizeof(OfferRemovalReceipt));
	}

	TEST(TEST_CLASS, CanCreateOfferRemovalReceipt) {
		// Arrange:
		auto sender = test::GenerateRandomByteArray<Key>();

		// Act:
		OfferRemovalReceipt receipt(static_cast<ReceiptType>(128), sender, std::pair<MosaicId,MosaicId>(35,58), Amount(358));

		// Assert:
		ASSERT_EQ(sizeof(OfferRemovalReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(static_cast<ReceiptType>(128), receipt.Type);
		EXPECT_EQ(MosaicId(35), receipt.MosaicsPair.first);
		EXPECT_EQ(MosaicId(58), receipt.MosaicsPair.second);
		EXPECT_EQ(Amount(358), receipt.AmountGiveReturned);
	}

	// endregion
}}
