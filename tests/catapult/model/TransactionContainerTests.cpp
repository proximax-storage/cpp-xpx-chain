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

#include "catapult/model/TransactionContainer.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionContainerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	namespace {
#pragma pack(push, 1)

		// emulates BlockHeader
		struct ContainerHeader : public SizePrefixedEntity {
		public:
			uint32_t Tag;

		public:
			size_t GetHeaderSize() const {
				return sizeof(ContainerHeader);
			}
		};

		// emulates Transaction (intentionally has different size than ContainerHeader)
		struct ContainerComponent : public SizePrefixedEntity {
			uint16_t Id;
		};

#pragma pack(pop)

		// emulates Block
		struct Container : public TransactionContainer<ContainerHeader, ContainerComponent> {};

		size_t GetTransactionPayloadSize(const ContainerHeader& header) {
			return header.Size - sizeof(ContainerHeader);
		}

		model::UniqueEntityPtr<Container> CreateContainer(uint32_t extraSize, std::initializer_list<uint32_t> attachmentExtraSizes) {
			uint32_t size = sizeof(ContainerHeader) + extraSize;
			for (auto attachmentExtraSize : attachmentExtraSizes)
				size += sizeof(ContainerComponent) + attachmentExtraSize;

			auto pContainer = utils::MakeUniqueWithSize<Container>(size);
			pContainer->Size = size;

			auto* pData = reinterpret_cast<uint8_t*>(pContainer.get() + 1);
			for (auto attachmentExtraSize : attachmentExtraSizes) {
				uint32_t attachmentSize = sizeof(ContainerComponent) + attachmentExtraSize;
				reinterpret_cast<ContainerComponent*>(pData)->Size = attachmentSize;
				pData += attachmentSize;
			}

			return pContainer;
		}

		// region traits

		using ConstTraits = test::ConstTraitsT<Container>;
		using NonConstTraits = test::NonConstTraitsT<Container>;

		// endregion
	}

#define TEST_CLASS TransactionContainerTests

#define DATA_POINTER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Const) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ConstTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_NonConst) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonConstTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region valid

	DATA_POINTER_TEST(TransactionsAreInaccessibleWhenContainerHasNoTransactions) {
		// Arrange:
		auto pContainer = CreateContainer(0, {});
		auto& accessor = TTraits::GetAccessor(*pContainer);

		// Act + Assert:
		EXPECT_FALSE(!!accessor.TransactionsPtr());
		EXPECT_EQ(0u, test::CountTransactions(accessor.Transactions()));
	}

	DATA_POINTER_TEST(TransactionsAreAccessibleWhenContainerHasTransactionsWithSizesEqualToPayloadSize) {
		// Arrange:
		auto pContainer = CreateContainer(0, { 100, 50, 75 });
		const auto* pContainerEnd = test::AsVoidPointer(pContainer.get() + 1);
		auto& accessor = TTraits::GetAccessor(*pContainer);

		// Act + Assert:
		EXPECT_EQ(pContainerEnd, accessor.TransactionsPtr());
		EXPECT_EQ(3u, test::CountTransactions(accessor.Transactions()));
	}

	// endregion

	// region invalid

	DATA_POINTER_TEST(TransactionsAreInaccessibleWhenReportedSizeIsLessThanContainerHeaderSize) {
		// Arrange:
		auto pContainer = CreateContainer(0, {});
		--pContainer->Size;
		auto& accessor = TTraits::GetAccessor(*pContainer);

		// Act + Assert:
		EXPECT_FALSE(!!accessor.TransactionsPtr());
		EXPECT_EQ(0u, test::CountTransactions(accessor.Transactions()));
	}

	DATA_POINTER_TEST(TransactionsArePartiallyAccessibleWhenContainerHasTransactionsWithSizesNotEqualToPayloadSize) {
		// Arrange:
		auto pContainer = CreateContainer(0, { 100, 50, 75 });
		--pContainer->Size;
		auto& accessor = TTraits::GetAccessor(*pContainer);

		// Act + Assert:
		EXPECT_TRUE(!!accessor.TransactionsPtr());
		EXPECT_EQ(2u, test::CountTransactions(accessor.Transactions(EntityContainerErrorPolicy::Suppress)));
		EXPECT_THROW(test::CountTransactions(accessor.Transactions()), catapult_runtime_error);
	}

	// endregion
}}
