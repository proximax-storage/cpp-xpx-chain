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

#include "src/model/AggregateTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionContainerTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS AggregateTransactionTests

	// region size + properties

	TEST(TEST_CLASS, EntityV1HasExpectedSize){
		// Arrange:
		auto expectedSize =
				sizeof(Transaction) // base
				+ sizeof(uint32_t); // payload size

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(AggregateTransaction<AggregateTransactionRawDescriptor>));
		EXPECT_EQ(122u + 4, sizeof(AggregateTransaction<AggregateTransactionRawDescriptor>));
	}

	TEST(TEST_CLASS, EntityV2HasExpectedSize){
		// Arrange:
		auto expectedSize =
				sizeof(Transaction) // base
				+ sizeof(uint32_t); // payload size

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(AggregateTransaction<AggregateTransactionExtendedDescriptor>));
		EXPECT_EQ(122u + 4, sizeof(AggregateTransaction<AggregateTransactionExtendedDescriptor>));
	}

	TEST(TEST_CLASS, TransactionV1HasExpectedProperties) {
		// Assert:
		EXPECT_EQ(Entity_Type_Aggregate_Complete_V1, AggregateTransaction<AggregateTransactionRawDescriptor>::Entity_Type);
		EXPECT_EQ(3u, AggregateTransaction<AggregateTransactionRawDescriptor>::Current_Version);
	}

	TEST(TEST_CLASS, TransactionV2HasExpectedProperties) {
		// Assert:
		EXPECT_EQ(Entity_Type_Aggregate_Complete_V2, AggregateTransaction<AggregateTransactionExtendedDescriptor>::Entity_Type);
		EXPECT_EQ(1u, AggregateTransaction<AggregateTransactionExtendedDescriptor>::Current_Version);
	}

	// endregion

	// region test utils

	namespace {
		using EmbeddedTransactionType = mocks::EmbeddedMockTransaction;

		template<typename TDescriptor>
		auto CreateAggregateTransaction(
				uint32_t extraSize,
				std::initializer_list<uint16_t> attachmentExtraSizes) {
			uint32_t size = sizeof(AggregateTransaction<TDescriptor>) + extraSize;
			for (auto attachmentExtraSize : attachmentExtraSizes)
				size += sizeof(EmbeddedTransactionType) + attachmentExtraSize;

			auto pTransaction = utils::MakeUniqueWithSize<AggregateTransaction<TDescriptor>>(size);
			pTransaction->Size = size;
			pTransaction->PayloadSize = size - (sizeof(AggregateTransaction<TDescriptor>) + extraSize);

			auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
			for (auto attachmentExtraSize : attachmentExtraSizes) {
				auto pEmbeddedTransaction = reinterpret_cast<EmbeddedTransactionType*>(pData);
				pEmbeddedTransaction->Size = sizeof(EmbeddedTransactionType) + attachmentExtraSize;
				pEmbeddedTransaction->Type = EmbeddedTransactionType::Entity_Type;
				pEmbeddedTransaction->Data.Size = attachmentExtraSize;
				pData += pEmbeddedTransaction->Size;
			}

			return pTransaction;
		}

		template<typename TDescriptor>
		EmbeddedTransactionType& GetSecondTransaction(AggregateTransaction<TDescriptor>& transaction) {
			uint8_t* pBytes = reinterpret_cast<uint8_t*>(transaction.TransactionsPtr());
			return *reinterpret_cast<EmbeddedTransactionType*>(pBytes + transaction.TransactionsPtr()->Size);
		}
	}

	// endregion

	// region transactions

	namespace {
		template<typename TDescriptor>
		struct ConstTraits : public test::ConstTraitsT<AggregateTransaction<TDescriptor>> {
			using Descriptor = TDescriptor;
		};
		template<typename TDescriptor>
		struct NonConstTraits : public test::NonConstTraitsT<AggregateTransaction<TDescriptor>>{
			using Descriptor = TDescriptor;
		};
	}

#define DATA_POINTER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Const) { \
		TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ConstTraits<model::AggregateTransactionRawDescriptor>>();                   \
		TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ConstTraits<model::AggregateTransactionExtendedDescriptor>>();				\
	} \
	TEST(TEST_CLASS, TEST_NAME##_NonConst) {                                   \
		TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonConstTraits<model::AggregateTransactionRawDescriptor>>();                \
		TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonConstTraits<model::AggregateTransactionExtendedDescriptor>>();                 \
	} \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	DATA_POINTER_TEST(TransactionsAreInaccessibleWhenAggregateHasNoTransactions) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(0, {});
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_FALSE(!!accessor.TransactionsPtr());
		EXPECT_EQ(0u, test::CountTransactions(accessor.Transactions()));
	}

	DATA_POINTER_TEST(TransactionsAreAccessibleWhenAggregateHasTransactions) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(0, { 1, 2, 3 });
		const auto* pTransactionEnd = test::AsVoidPointer(pTransaction.get() + 1);
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_EQ(pTransactionEnd, accessor.TransactionsPtr());
		EXPECT_EQ(3u, test::CountTransactions(accessor.Transactions()));
	}

	// endregion

	// region cosignatures

	DATA_POINTER_TEST(CosignaturesAreInacessibleWhenReportedSizeIsLessThanHeaderSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(0, {});
		--pTransaction->Size;
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_EQ(0u, accessor.CosignaturesCount());
		EXPECT_FALSE(!!accessor.CosignaturesPtr());
	}

	DATA_POINTER_TEST(CosignaturesAreInacessibleWhenThereAreNoCosignatures) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(0, {});
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_EQ(0u, accessor.CosignaturesCount());
		EXPECT_FALSE(!!accessor.CosignaturesPtr());
	}

	DATA_POINTER_TEST(CosignaturesAreAccessibleWhenThereAreNoTransactionsButCosignatures) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(2 * sizeof(typename TTraits::Descriptor::CosignatureType), {});
		const auto* pAggregateEnd = reinterpret_cast<const uint8_t*>(pTransaction.get() + 1);
		const auto* pTransactionsEnd = test::AsVoidPointer(pAggregateEnd + pTransaction->PayloadSize);
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_EQ(2u, accessor.CosignaturesCount());
		EXPECT_EQ(pTransactionsEnd, accessor.CosignaturesPtr());

		// Sanity:
		EXPECT_EQ(pAggregateEnd, pTransactionsEnd);
	}

	DATA_POINTER_TEST(CosignaturesAreAccessibleWhenThereAreTransactionsAndCosignatures) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(sizeof(typename TTraits::Descriptor::CosignatureType), { 1, 2, 3 });
		const auto* pAggregateEnd = reinterpret_cast<const uint8_t*>(pTransaction.get() + 1);
		const auto* pTransactionsEnd = test::AsVoidPointer(pAggregateEnd + pTransaction->PayloadSize);
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert:
		EXPECT_EQ(1u, accessor.CosignaturesCount());
		EXPECT_EQ(pTransactionsEnd, accessor.CosignaturesPtr());

		// Sanity:
		EXPECT_NE(pAggregateEnd, pTransactionsEnd);
	}

	DATA_POINTER_TEST(CosignaturesAreAccessibleWhenThereAreTransactionsAndPartialCosignatures) {
		// Arrange: three transactions and space for 2.5 cosignatures
		auto pTransaction = CreateAggregateTransaction<typename TTraits::Descriptor>(2 * sizeof(typename TTraits::Descriptor::CosignatureType) + sizeof(typename TTraits::Descriptor::CosignatureType) / 2, { 1, 2, 3 });
		const auto* pAggregateEnd = reinterpret_cast<const uint8_t*>(pTransaction.get() + 1);
		const auto* pTransactionsEnd = test::AsVoidPointer(pAggregateEnd + pTransaction->PayloadSize);
		auto& accessor = TTraits::GetAccessor(*pTransaction);

		// Act + Assert: two cosignatures should be exposed
		// - unlike in most transaction types, cosignature accessors assume PayloadSize is correct instead of returning nullptr
		// - this is because aggregate transaction size cannot be calculated without a registry, which is excessive for accessors
		EXPECT_EQ(2u, accessor.CosignaturesCount());
		EXPECT_EQ(pTransactionsEnd, accessor.CosignaturesPtr());

		// Sanity:
		EXPECT_NE(pAggregateEnd, pTransactionsEnd);
	}

	// endregion

	// region GetTransactionPayloadSize

	TEST(TEST_CLASS, GetTransactionV1PayloadSizeReturnsCorrectPayloadSize) {
		// Arrange:
		AggregateTransactionHeader<AggregateTransactionRawDescriptor> header;
		header.PayloadSize = 123;

		// Act:
		auto payloadSize = GetTransactionPayloadSize(header);

		// Assert:
		EXPECT_EQ(123u, payloadSize);
	}

	TEST(TEST_CLASS, GetTransactionV2PayloadSizeReturnsCorrectPayloadSize) {
		// Arrange:
		AggregateTransactionHeader<AggregateTransactionExtendedDescriptor> header;
		header.PayloadSize = 123;

		// Act:
		auto payloadSize = GetTransactionPayloadSize(header);

		// Assert:
		EXPECT_EQ(123u, payloadSize);
	}

	// endregion

	namespace {
		template<typename TDescriptor>
		bool IsSizeValid(const AggregateTransaction<TDescriptor>& transaction, mocks::PluginOptionFlags options = mocks::PluginOptionFlags::Default) {
			auto registry = mocks::CreateDefaultTransactionRegistry(options);
			return IsSizeValid(transaction, registry);
		}
	}

	// region IsSizeValid - no transactions

	TEST(TEST_CLASS, V1SizeInvalidWhenReportedSizeIsZero) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, {});
		pTransaction->Size = 0;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenReportedSizeIsLessThanHeaderSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, {});
		--pTransaction->Size;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeValidWhenReportedSizeIsEqualToHeaderSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, {});

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenReportedSizeIsZero) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, {});
		pTransaction->Size = 0;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenReportedSizeIsLessThanHeaderSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, {});
		--pTransaction->Size;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeValidWhenReportedSizeIsEqualToHeaderSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, {});

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pTransaction));
	}

	// endregion

	// region IsSizeValid - invalid inner tx sizes

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyTransactionHasPartialHeader) {
		// Arrange: create an aggregate with 1 extra byte (which can be interpeted as a partial tx header)
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(1, {});

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyTransactionHasInvalidSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Data.Size = 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyTransactionHasZeroSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Size = 0;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyInnerTransactionExpandsBeyondBuffer) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Size = pTransaction->Size - pTransaction->TransactionsPtr()->Size + 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyTransactionHasPartialHeader) {
		// Arrange: create an aggregate with 1 extra byte (which can be interpeted as a partial tx header)
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(1, {});

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyTransactionHasInvalidSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Data.Size = 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyTransactionHasZeroSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Size = 0;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyInnerTransactionExpandsBeyondBuffer) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Size = pTransaction->Size - pTransaction->TransactionsPtr()->Size + 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	// endregion

	// region IsSizeValid - invalid inner tx types

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyTransactionHasUnknownType) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Type = static_cast<EntityType>(-1);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenAnyTransactionDoesNotSupportEmbedding) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction, mocks::PluginOptionFlags::Not_Embeddable));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyTransactionHasUnknownType) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });
		GetSecondTransaction(*pTransaction).Type = static_cast<EntityType>(-1);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenAnyTransactionDoesNotSupportEmbedding) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction, mocks::PluginOptionFlags::Not_Embeddable));
	}

	// endregion

	// region IsSizeValid - payload size

	TEST(TEST_CLASS, V1SizeInvalidWhenPayloadSizeIsTooLargeRelativeToSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });
		++pTransaction->PayloadSize;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeInvalidWhenSpaceForCosignaturesIsNotMultipleOfCosignatureSize) {
		// Arrange:
		for (auto extraSize : { 1u, 3u, static_cast<uint32_t>(sizeof(AggregateTransactionRawDescriptor::CosignatureType) - 1) }) {
			// - add extra bytes, which will cause space to not be multiple of cosignature size
			auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(2 * sizeof(AggregateTransactionRawDescriptor::CosignatureType) + extraSize, { 1, 2, 3 });

			// Act + Assert:
			EXPECT_FALSE(IsSizeValid(*pTransaction)) << "extra size: " << extraSize;
		}
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenPayloadSizeIsTooLargeRelativeToSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });
		++pTransaction->PayloadSize;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeInvalidWhenSpaceForCosignaturesIsNotMultipleOfCosignatureSize) {
		// Arrange:
		for (auto extraSize : { 1u, 3u, static_cast<uint32_t>(sizeof(AggregateTransactionExtendedDescriptor::CosignatureType) - 1) }) {
			// - add extra bytes, which will cause space to not be multiple of cosignature size
			auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(2 * sizeof(AggregateTransactionExtendedDescriptor::CosignatureType) + extraSize, { 1, 2, 3 });

			// Act + Assert:
			EXPECT_FALSE(IsSizeValid(*pTransaction)) << "extra size: " << extraSize;
		}
	}

	// endregion

	// region IsSizeValid - valid transactions

	TEST(TEST_CLASS, V1SizeValidWhenReportedSizeIsEqualToHeaderSizePlusTransactionsSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(0, { 1, 2, 3 });

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V1SizeValidWhenReportedSizeIsEqualToHeaderSizePlusTransactionsSizeAndHasSpaceForCosignatures) {
		// Arrange:
		for (auto numCosignatures : { 1u, 3u }) {
			// Arrange:
			auto pTransaction = CreateAggregateTransaction<AggregateTransactionRawDescriptor>(numCosignatures * sizeof(AggregateTransactionRawDescriptor::CosignatureType), { 1, 2, 3 });

			// Act + Assert:
			EXPECT_TRUE(IsSizeValid(*pTransaction)) << "num cosignatures: " << numCosignatures;
		}
	}

	TEST(TEST_CLASS, V2SizeValidWhenReportedSizeIsEqualToHeaderSizePlusTransactionsSize) {
		// Arrange:
		auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(0, { 1, 2, 3 });

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pTransaction));
	}

	TEST(TEST_CLASS, V2SizeValidWhenReportedSizeIsEqualToHeaderSizePlusTransactionsSizeAndHasSpaceForCosignatures) {
		// Arrange:
		for (auto numCosignatures : { 1u, 3u }) {
			// Arrange:
			auto pTransaction = CreateAggregateTransaction<AggregateTransactionExtendedDescriptor>(numCosignatures * sizeof(AggregateTransactionExtendedDescriptor::CosignatureType), { 1, 2, 3 });

			// Act + Assert:
			EXPECT_TRUE(IsSizeValid(*pTransaction)) << "num cosignatures: " << numCosignatures;
		}
	}

	// endregion
}}
