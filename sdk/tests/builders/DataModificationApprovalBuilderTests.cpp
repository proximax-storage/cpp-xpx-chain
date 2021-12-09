/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/builders/DataModificationApprovalBuilder.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS DataModificationApprovalTests

    namespace {
        using RegularTraits = test::RegularTransactionTraits<model::DataModificationApprovalTransaction>;
        using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedDataModificationApprovalTransaction>;

        struct TransactionProperties {
        public:
            TransactionProperties()
                    : DriveKey(Key{})
                    , DataModificationId(Hash256{})
                    , FileStructureCdi(Hash256{})
                    , FileStructureSize(0)
                    , UsedDriveSize(0)
            {}

        public:
            Key DriveKey;
            Hash256 DataModificationId;
            Hash256 FileStructureCdi;
            uint64_t FileStructureSize;
            uint64_t UsedDriveSize;
        };

        template<typename TTransaction>
        void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
            EXPECT_EQ(expectedProperties.DriveKey, transaction.DriveKey);
            EXPECT_EQ(expectedProperties.DataModificationId, transaction.DataModificationId);
            EXPECT_EQ(expectedProperties.FileStructureCdi, transaction.FileStructureCdi);
            EXPECT_EQ(expectedProperties.FileStructureSize, transaction.FileStructureSize);
            EXPECT_EQ(expectedProperties.UsedDriveSize, transaction.UsedDriveSize);
        }

        template<typename TTraits>
        void AssertCanBuildTransaction(
                size_t propertiesSize,
                const TransactionProperties& expectedProperties,
                const consumer<DataModificationApprovalBuilder&>& buildTransaction) {
            // Arrange:
            auto networkId = static_cast<model::NetworkIdentifier>(0x62);
            auto signer = test::GenerateRandomByteArray<Key>();

            // Act:
            DataModificationApprovalBuilder builder(networkId, signer);
            buildTransaction(builder);
            auto pTransaction = TTraits::InvokeBuilder(builder);

            // Assert:
            TTraits::CheckFields(propertiesSize, *pTransaction);
            EXPECT_EQ(signer, pTransaction->Signer);
            EXPECT_EQ(0x62000001, pTransaction->Version);
            EXPECT_EQ(model::Entity_Type_DataModificationApproval, pTransaction->Type);

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
        // Assert:
        AssertCanBuildTransaction<TTraits>(0, TransactionProperties(), [](auto& builder) {
            builder.setFileStructureSize(0);
            builder.setUsedDriveSize(0);
        });
    }

    // endregion

    // region required properties

    TRAITS_BASED_TEST(CanSetDriveKey) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        auto expectedProperties = TransactionProperties();
        expectedProperties.DriveKey = driveKey;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setDriveKey(driveKey);
            builder.setFileStructureSize(0);
            builder.setUsedDriveSize(0);
        });
    }

    TRAITS_BASED_TEST(CanSetDataModificationId) {
        // Arrange:
        auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
        auto expectedProperties = TransactionProperties();
        expectedProperties.DataModificationId = dataModificationId;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setDataModificationId(dataModificationId);
            builder.setFileStructureSize(0);
            builder.setUsedDriveSize(0);
        });
    }

    TRAITS_BASED_TEST(CanSetFileStructureCdi) {
        // Arrange:
        auto fileStructureCdi = test::GenerateRandomByteArray<Hash256>();

        auto expectedProperties = TransactionProperties();
        expectedProperties.FileStructureCdi = fileStructureCdi;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setFileStructureCdi(fileStructureCdi);
            builder.setFileStructureSize(0);
            builder.setUsedDriveSize(0);
        });
    }

    TRAITS_BASED_TEST(CanSetFileStructureSize) {
        // Arrange:
        auto fileStructureSize = 100;
        auto expectedProperties = TransactionProperties();
        expectedProperties.FileStructureSize = fileStructureSize;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setFileStructureSize(fileStructureSize);
            builder.setUsedDriveSize(0);
        });
    }

    TRAITS_BASED_TEST(CanSetUsedDriveSize) {
        // Arrange:
        auto usedDriveSize = 100;
        auto expectedProperties = TransactionProperties();
        expectedProperties.UsedDriveSize = usedDriveSize;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setFileStructureSize(0);
            builder.setUsedDriveSize(usedDriveSize);
        });
    }

    // endregion
}}
