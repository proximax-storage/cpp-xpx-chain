/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/NamespaceMetadataTransaction.h"
#include "tests/test/core/TransactionContainerTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/MetadataTestUtils.h"

namespace catapult { namespace model {

        using TransactionType = NamespaceMetadataTransaction;

#define TEST_CLASS NamespaceMetadataTransactionTests

        // region size + properties

        namespace {
            template<typename T>
            void AssertEntityHasExpectedSize(size_t baseSize) {
                // Arrange:
                auto expectedSize = baseSize // base
                    + sizeof(model::MetadataType) // MetadataType
                    + sizeof(NamespaceId); // MetadataId

                // Assert:
                EXPECT_EQ(expectedSize, sizeof(T));
                EXPECT_EQ(baseSize + 9u, sizeof(T));
            }

            template<typename T>
            void AssertTransactionHasExpectedProperties() {
                // Assert:
                EXPECT_EQ(Entity_Type_Namespace_Metadata, static_cast<EntityType>(T::Entity_Type));
                EXPECT_EQ(1u, static_cast<VersionType>(T::Current_Version));
            }
        }

        ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(NamespaceMetadata)

        // endregion

        // region transactions

        namespace {
            using ConstTraits = test::ConstTraitsT<TransactionType>;
            using NonConstTraits = test::NonConstTraitsT<TransactionType>;
        }

#define DATA_POINTER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Const) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ConstTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_NonConst) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonConstTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    DATA_POINTER_TEST(ModificationsAreInaccessibleWhenTransactionHasNoModifications) {
        // Arrange:
        auto pTransaction = test::CreateTransaction<TransactionType>({});
        auto& accessor = TTraits::GetAccessor(*pTransaction);

        // Act + Assert:
        EXPECT_FALSE(!!accessor.TransactionsPtr());
        EXPECT_EQ(0u, test::CountTransactions(accessor.Transactions()));
    }

    DATA_POINTER_TEST(ModificationsAreInacessibleIfReportedSizeIsLessThanHeaderSize) {
        // Arrange:
        auto pTransaction = test::CreateTransaction<TransactionType>({});
        --pTransaction->Size;
        auto& accessor = TTraits::GetAccessor(*pTransaction);

        // Act + Assert:
        EXPECT_FALSE(!!accessor.TransactionsPtr());
    }

    DATA_POINTER_TEST(ModificationsAreAccessibleWhenTransactionHasModifications) {
        // Arrange:
        auto pTransaction = test::CreateTransaction<TransactionType>({
            test::CreateModification(MetadataModificationType::Add, 1, 2).get(),
            test::CreateModification(MetadataModificationType::Del, 3, 4).get(),
            test::CreateModification(MetadataModificationType::Add, 5, 6).get()
        });
        const auto* pTransactionEnd = test::AsVoidPointer(pTransaction.get() + 1);
        auto& accessor = TTraits::GetAccessor(*pTransaction);

        // Act + Assert:
        EXPECT_EQ(pTransactionEnd, accessor.TransactionsPtr());
        EXPECT_EQ(3u, test::CountTransactions(accessor.Transactions()));
    }

    // endregion

    // region GetTransactionPayloadSize

    TEST(TEST_CLASS, GetTransactionPayloadSizeReturnsCorrectPayloadSize) {
        // Arrange:
        uint32_t entitySize = sizeof(TransactionType) + 123u;
        auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
        pTransaction->Size = entitySize;

        // Act:
        auto payloadSize = GetTransactionPayloadSize(*pTransaction);

        // Assert:
        EXPECT_EQ(123u, payloadSize);
    }

    // endregion

    // region CalculateRealSize

    TEST(TEST_CLASS, CalculateRealSizeWithWrongModificationSize) {
        // Arrange:
        auto pInvalidModification = test::CreateModification(MetadataModificationType::Add, 1, 2);
        pInvalidModification->Size--;
        auto pTransaction = test::CreateTransaction<TransactionType>({
            pInvalidModification.get(),
            test::CreateModification(MetadataModificationType::Del, 3, 4).get(),
            test::CreateModification(MetadataModificationType::Add, 5, 6).get()
        });

        // Act:
        auto realSize = TransactionType::CalculateRealSize(*pTransaction);

        // Assert:
        EXPECT_EQ(std::numeric_limits<uint64_t>::max(), realSize);
    }

    TEST(TEST_CLASS, CalculateRealSizeWithValidModificationSizes) {
        // Arrange:
        auto pTransaction = test::CreateTransaction<TransactionType>({
            test::CreateModification(MetadataModificationType::Add, 1, 2).get(),
            test::CreateModification(MetadataModificationType::Del, 3, 4).get(),
            test::CreateModification(MetadataModificationType::Add, 5, 6).get()
         });

        // Act:
        auto realSize = TransactionType::CalculateRealSize(*pTransaction);

        // Assert:
        EXPECT_EQ(176u, realSize);
    }

    // endregion
}}
