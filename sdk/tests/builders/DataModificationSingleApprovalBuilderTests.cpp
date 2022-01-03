/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/builders/DataModificationSingleApprovalBuilder.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS DataModificationSingleApprovalTests

    namespace {
        using RegularTraits = test::RegularTransactionTraits<model::DataModificationSingleApprovalTransaction>;
        using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedDataModificationSingleApprovalTransaction>;

        struct TransactionProperties {
        public:
            TransactionProperties()
                    : DriveKey(Key{})
                      , DataModificationId(Hash256{})
                      , PublicKeys({})
                      , Opinions({}) {}

        public:
            Key DriveKey;
            Hash256 DataModificationId;
            std::vector<Key> PublicKeys;
            std::vector<uint64_t> Opinions;
        };

        template<typename TTransaction>
        void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
            EXPECT_EQ(expectedProperties.DriveKey, transaction.DriveKey);
            EXPECT_EQ(expectedProperties.DataModificationId, transaction.DataModificationId);
            EXPECT_EQ(expectedProperties.PublicKeys.size(), transaction.PublicKeysCount);

            auto uploaderKeysPtr = transaction.PublicKeysPtr();
            for (const auto& key: expectedProperties.PublicKeys) {
                auto property = *uploaderKeysPtr;
                EXPECT_EQ(property, key);
                ++uploaderKeysPtr;
            }

            auto uploadOpinionPtr = transaction.OpinionsPtr();
            for (auto opinion: expectedProperties.Opinions) {
                auto expectedOpinion = *uploadOpinionPtr;
                EXPECT_EQ(expectedOpinion, opinion);
                ++uploadOpinionPtr;
            }
        }

        template<typename TTraits>
        void AssertCanBuildTransaction(
                size_t propertiesSize,
                const TransactionProperties& expectedProperties,
                const consumer<DataModificationSingleApprovalBuilder&>& buildTransaction) {
            // Arrange:
            auto networkId = static_cast<model::NetworkIdentifier>(0x62);
            auto signer = test::GenerateRandomByteArray<Key>();

            // Act:
            DataModificationSingleApprovalBuilder builder(networkId, signer);
            buildTransaction(builder);
            auto pTransaction = TTraits::InvokeBuilder(builder);

            // Assert:
            TTraits::CheckFields(propertiesSize, *pTransaction);
            EXPECT_EQ(signer, pTransaction->Signer);
            EXPECT_EQ(0x62000001, pTransaction->Version);
            EXPECT_EQ(model::Entity_Type_DataModificationSingleApproval, pTransaction->Type);

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
        AssertCanBuildTransaction<TTraits>(0, TransactionProperties(), [](auto& builder) {});
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
        });
    }

    TRAITS_BASED_TEST(CanSetPublicKeysAndOpinions) {
        // Arrange:
        std::vector<Key> publicKeys{
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>()
        };

        auto expectedProperties = TransactionProperties();
        expectedProperties.PublicKeys = publicKeys;

        std::vector<uint64_t> opinions{1, 1, 1};
        expectedProperties.Opinions = opinions;

        auto additionalSize = publicKeys.size() * sizeof(Key) + opinions.size() * sizeof(uint64_t);

        // Assert:
        AssertCanBuildTransaction<TTraits>(additionalSize, expectedProperties, [&](auto& builder) {
            builder.setPublicKeys(std::move(publicKeys));
            builder.setOpinions(std::move(opinions));
        });
    }

    // endregion
}}
