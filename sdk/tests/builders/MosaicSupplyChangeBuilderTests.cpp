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

#include "src/builders/MosaicSupplyChangeBuilder.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS MosaicSupplyChangeBuilderTests

	namespace {
		using RegularTraits = test::RegularTransactionTraits<model::MosaicSupplyChangeTransaction>;
		using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedMosaicSupplyChangeTransaction>;

		using TransactionType = model::MosaicSupplyChangeTransaction;
		using TransactionPtr = std::unique_ptr<TransactionType>;

		struct TransactionProperties {
		public:
			explicit TransactionProperties(UnresolvedMosaicId mosaicId)
					: MosaicId(mosaicId)
					, Direction(model::MosaicSupplyChangeDirection::Decrease)
			{}

		public:
			UnresolvedMosaicId MosaicId;
			model::MosaicSupplyChangeDirection Direction;
			Amount Delta;
		};

		template<typename TTransaction>
		void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
			EXPECT_EQ(expectedProperties.MosaicId, transaction.MosaicId);
			EXPECT_EQ(expectedProperties.Direction, transaction.Direction);
			EXPECT_EQ(expectedProperties.Delta, transaction.Delta);
		}

		template<typename TTraits>
		void AssertCanBuildTransaction(
				const TransactionProperties& expectedProperties,
				const consumer<MosaicSupplyChangeBuilder&>& buildTransaction) {
			// Arrange:
			auto networkId = static_cast<model::NetworkIdentifier>(0x62);
			auto signer = test::GenerateRandomByteArray<Key>();

			// Act:
			auto builder = MosaicSupplyChangeBuilder(networkId, signer);
			buildTransaction(builder);
			auto pTransaction = TTraits::InvokeBuilder(builder);

			// Assert:
			TTraits::CheckFields(0, *pTransaction);
			EXPECT_EQ(signer, pTransaction->Signer);
			EXPECT_EQ(0x62000002, pTransaction->Version);
			EXPECT_EQ(model::Entity_Type_Mosaic_Supply_Change, pTransaction->Type);

			AssertTransactionProperties(expectedProperties, *pTransaction);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Regular) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Embedded) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region constructor

	TRAITS_BASED_TEST(CanCreateTransaction) {
		// Arrange:
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		auto expectedProperties = TransactionProperties(mosaicId);

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [mosaicId](auto& builder) {
			builder.setMosaicId(mosaicId);
		});
	}

	// endregion

	// region settings

	TRAITS_BASED_TEST(CanSetDirection) {
		// Arrange:
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		auto expectedProperties = TransactionProperties(mosaicId);
		expectedProperties.Direction = model::MosaicSupplyChangeDirection::Increase;

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [mosaicId](auto& builder) {
			builder.setMosaicId(mosaicId);
			builder.setDirection(model::MosaicSupplyChangeDirection::Increase);
		});
	}

	TRAITS_BASED_TEST(CanSetDelta) {
		// Arrange:
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		auto expectedProperties = TransactionProperties(mosaicId);
		expectedProperties.Delta = Amount(12345678);

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [mosaicId](auto& builder) {
			builder.setMosaicId(mosaicId);
			builder.setDelta(Amount(12345678));
		});
	}

	TRAITS_BASED_TEST(CanSetDirectonAndDelta) {
		// Arrange:
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		auto expectedProperties = TransactionProperties(mosaicId);
		expectedProperties.Direction = model::MosaicSupplyChangeDirection::Increase;
		expectedProperties.Delta = Amount(12345678);

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [mosaicId](auto& builder) {
			builder.setMosaicId(mosaicId);
			builder.setDirection(model::MosaicSupplyChangeDirection::Increase);
			builder.setDelta(Amount(12345678));
		});
	}

	// endregion
}}
