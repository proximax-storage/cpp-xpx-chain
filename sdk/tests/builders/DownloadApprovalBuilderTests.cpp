/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/builders/DownloadApprovalBuilder.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS DownloadApprovalTests

    namespace {
        using RegularTraits = test::RegularTransactionTraits<model::DownloadApprovalTransaction>;
        using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedDownloadApprovalTransaction>;

        struct TransactionProperties {
        public:
            TransactionProperties()
                    : DownloadChannelId(Hash256{})
                    , SequenceNumber(0)
                    , ResponseToFinishDownloadTransaction(false)
                    , JudgingKeysCount(0)
                    , OverlappingKeysCount(0)
                    , JudgedKeysCount(0)
                    , PublicKeys({})
                    , Signatures({})
                    , PresentOpinions({})
                    , Opinions({}) {}

        public:
            Hash256 DownloadChannelId;
            uint16_t SequenceNumber;
            bool ResponseToFinishDownloadTransaction;
            uint8_t JudgingKeysCount;
            uint8_t OverlappingKeysCount;
            uint8_t JudgedKeysCount;
            std::vector<Key> PublicKeys;
            std::vector<Signature> Signatures;
            std::vector<uint8_t> PresentOpinions;
            std::vector<uint64_t> Opinions;
        };

        template<typename TTransaction>
        void
        AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
            EXPECT_EQ(expectedProperties.DownloadChannelId, transaction.DownloadChannelId);
            EXPECT_EQ(expectedProperties.SequenceNumber, transaction.SequenceNumber);
            EXPECT_EQ(expectedProperties.ResponseToFinishDownloadTransaction, transaction.ResponseToFinishDownloadTransaction);
            EXPECT_EQ(expectedProperties.JudgingKeysCount, transaction.JudgingKeysCount);
            EXPECT_EQ(expectedProperties.OverlappingKeysCount, transaction.OverlappingKeysCount);
            EXPECT_EQ(expectedProperties.JudgedKeysCount, transaction.JudgedKeysCount);

            auto publicKeysPtr = transaction.PublicKeysPtr();
            for (const auto& key: expectedProperties.PublicKeys) {
                auto property = *publicKeysPtr;
                EXPECT_EQ(property, key);
                ++publicKeysPtr;
            }

            auto signaturesPtr = transaction.SignaturesPtr();
            for (const auto& key: expectedProperties.Signatures) {
                auto property = *signaturesPtr;
                EXPECT_EQ(property, key);
                ++signaturesPtr;
            }

            auto presentOpinionsPtr = transaction.PresentOpinionsPtr();
            for (const auto& key: expectedProperties.PresentOpinions) {
                auto property = *presentOpinionsPtr;
                EXPECT_EQ(property, key);
                ++presentOpinionsPtr;
            }

            auto opinionsPtr = transaction.OpinionsPtr();
            for (const auto& key: expectedProperties.Opinions) {
                auto property = *opinionsPtr;
                EXPECT_EQ(property, key);
                ++opinionsPtr;
            }
        }

        template<typename TTraits>
        void AssertCanBuildTransaction(
                size_t propertiesSize,
                const TransactionProperties& expectedProperties,
                const consumer<DownloadApprovalBuilder&>& buildTransaction) {
            // Arrange:
            auto networkId = static_cast<model::NetworkIdentifier>(0x62);
            auto signer = test::GenerateRandomByteArray<Key>();

            // Act:
            DownloadApprovalBuilder builder(networkId, signer);
            buildTransaction(builder);
            auto pTransaction = TTraits::InvokeBuilder(builder);

            // Assert:
            TTraits::CheckFields(propertiesSize, *pTransaction);
            EXPECT_EQ(signer, pTransaction->Signer);
            EXPECT_EQ(0x62000001, pTransaction->Version);
            EXPECT_EQ(model::Entity_Type_DownloadApproval, pTransaction->Type);

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

    TRAITS_BASED_TEST(CanSetDownloadChannelId) {
        // Arrange:
        auto downloadChannelId = test::GenerateRandomByteArray<Hash256>();
        auto expectedProperties = TransactionProperties();
        expectedProperties.DownloadChannelId = downloadChannelId;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setDownloadChannelId(downloadChannelId);
        });
    }

    TRAITS_BASED_TEST(CanSetSequenceNumber) {
        // Arrange:
        auto sequenceNumber = 3;
        auto expectedProperties = TransactionProperties();
        expectedProperties.SequenceNumber = sequenceNumber;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setSequenceNumber(sequenceNumber);
        });
    }

    TRAITS_BASED_TEST(CanSetResponseToFinishDownloadTransaction) {
        // Arrange:
        auto responseToFinishDownloadTransaction = true;
        auto expectedProperties = TransactionProperties();
        expectedProperties.ResponseToFinishDownloadTransaction = responseToFinishDownloadTransaction;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setResponseToFinishDownloadTransaction(responseToFinishDownloadTransaction);
        });
    }

    TRAITS_BASED_TEST(CanSetJudgingKeysCount) {
        // Arrange:
        auto judgingKeysCount = 100;
        auto expectedProperties = TransactionProperties();
        expectedProperties.JudgingKeysCount = judgingKeysCount;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setJudgingKeysCount(judgingKeysCount);
        });
    }

    TRAITS_BASED_TEST(CanSetOverlappingKeysCount) {
        // Arrange:
        auto overlappingKeysCount = 100;
        auto expectedProperties = TransactionProperties();
        expectedProperties.OverlappingKeysCount = overlappingKeysCount;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setOverlappingKeysCount(overlappingKeysCount);
        });
    }

    TRAITS_BASED_TEST(CanSetJudgedKeysCount) {
        // Arrange:
        auto judgedKeysCount = 100;
        auto expectedProperties = TransactionProperties();
        expectedProperties.JudgedKeysCount = judgedKeysCount;

        // Assert:
        AssertCanBuildTransaction<TTraits>(0, expectedProperties, [&](auto& builder) {
            builder.setJudgedKeysCount(judgedKeysCount);
        });
    }

//    TRAITS_BASED_TEST(CanSetPublicKeysAndSignaturesAndPresentOpinions) {
//        // Arrange:
//        std::vector<Key> publicKeys{
//                test::GenerateRandomByteArray<Key>(),
//                test::GenerateRandomByteArray<Key>(),
//        };
//        auto expectedProperties = TransactionProperties();
//        expectedProperties.PublicKeys = publicKeys;
//        expectedProperties.JudgingKeysCount = publicKeys.size();
//
//        std::vector<Signature> signatures{test::GenerateRandomByteArray<Signature>()};
//        expectedProperties.Signatures = signatures;
//        expectedProperties.JudgingKeysCount = signatures.size();
//        expectedProperties.JudgedKeysCount = signatures.size();
//
//        std::vector<uint8_t> presentOpinions{3}; //0000011
//        expectedProperties.PresentOpinions = presentOpinions;
//
//        auto additionalSize = publicKeys.size() * sizeof(Key) + signatures.size() * sizeof(Signature) + presentOpinions.size() * sizeof(uint8_t);
//
//        // Assert:
//        AssertCanBuildTransaction<TTraits>(additionalSize, expectedProperties, [&](auto& builder) {
//            builder.setPublicKeys(std::move(publicKeys));
//            builder.setSignatures(std::move(signatures));
//            builder.setJudgingKeysCount(signatures.size());
//            builder.setJudgedKeysCount(signatures.size());
//            builder.setPresentOpinions(std::move(presentOpinions));
//        });
//    }

    TRAITS_BASED_TEST(CanSetOpinions) {
        // Arrange:
        std::vector<uint64_t> opinions{0, 1, 2};
        auto expectedProperties = TransactionProperties();
        expectedProperties.Opinions = opinions;

        auto additionalSize = opinions.size() * sizeof(uint64_t);

        // Assert:
        AssertCanBuildTransaction<TTraits>(additionalSize, expectedProperties, [&](auto& builder) {
            builder.setOpinions(std::move(opinions));
        });
    }

    // endregion
}}
