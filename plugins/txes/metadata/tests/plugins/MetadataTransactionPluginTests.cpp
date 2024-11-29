/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/AddressMetadataV1Transaction.h"
#include "src/model/MetadataV1Notifications.h"
#include "src/model/MosaicMetadataV1Transaction.h"
#include "src/model/NamespaceMetadataV1Transaction.h"
#include "src/plugins/MetadataV1TransactionPlugin.h"
#include "src/state/MetadataV1Utils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/MetadataTestUtils.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

namespace catapult { namespace plugins {

#define TEST_CLASS MetadataV1TransactionPluginTests

    namespace {
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(AddressMetadataV1, 1, 1, AddressMetadata)
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(MosaicMetadataV1, 1, 1, MosaicMetadata)
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(NamespaceMetadataV1, 1, 1, NamespaceMetadata)

        template<typename TTransaction, typename TTransactionTraits>
        struct AddressTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyAddressMetadataNotification_v1;
            using ModifyMetadataValueNotification = model::ModifyAddressMetadataValueNotification_v1;
        };

        using AddressRegularTraits = AddressTraits<model::AddressMetadataV1Transaction, AddressMetadataRegularTraits>;
        using AddressEmbeddedTraits = AddressTraits<model::EmbeddedAddressMetadataV1Transaction, AddressMetadataEmbeddedTraits>;

        template<typename TTransaction, typename TTransactionTraits>
        struct MosaicTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyMosaicMetadataNotification_v1;
            using ModifyMetadataValueNotification = model::ModifyMosaicMetadataValueNotification_v1;
        };

        using MosaicRegularTraits = MosaicTraits<model::MosaicMetadataV1Transaction, MosaicMetadataRegularTraits>;
        using MosaicEmbeddedTraits = MosaicTraits<model::EmbeddedMosaicMetadataV1Transaction, MosaicMetadataEmbeddedTraits>;

        template<typename TTransaction, typename TTransactionTraits>
        struct NamespaceTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyNamespaceMetadataNotification_v1;
            using ModifyMetadataValueNotification = model::ModifyNamespaceMetadataValueNotification_v1;
        };

        using NamespaceRegularTraits = NamespaceTraits<model::NamespaceMetadataV1Transaction, NamespaceMetadataRegularTraits>;
        using NamespaceEmbeddedTraits = NamespaceTraits<model::EmbeddedNamespaceMetadataV1Transaction, NamespaceMetadataEmbeddedTraits>;
    }

    template<typename TTraits>
    class MetadataTransactionPluginTests {
    public:
        // region TransactionPlugin

        static void AssertCanCalculateSize() {
            // Arrange:
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataV1ModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataV1ModificationType::Del, 3, 4).get()
            });

            // Act:
            auto realSize = pPlugin->calculateRealSize(*pTransaction);

            // Assert:
            EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 2 * sizeof(model::MetadataV1Modification) + 1 + 2 + 3 + 4, realSize);
        }

        // endregion

        // region metadata type notification

        static void AssertCanPublishMetadataTypeNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<model::MetadataV1TypeNotification<1>> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({});

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(2u, sub.numNotifications());
            ASSERT_EQ(1u, sub.numMatchingNotifications());
            EXPECT_EQ(pTransaction->MetadataType, sub.matchingNotifications()[0].MetadataType);
        }

        // endregion

        // region modify metadata notification

        static void AssertCanPublishModifyMetadataNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<typename TTraits::ModifyMetadataNotification> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({});

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(2u, sub.numNotifications());
            ASSERT_EQ(1u, sub.numMatchingNotifications());
            EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[0].Signer);
            EXPECT_EQ(pTransaction->MetadataId, sub.matchingNotifications()[0].MetadataId);
        }

        // endregion

        // region metadata modifications notification

        static void AssertCanPublishMetadataModificationsNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<model::MetadataV1ModificationsNotification<1>> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataV1ModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataV1ModificationType::Del, 3, 4).get()
            });
            auto metadataId = state::GetHash(state::ToVector(pTransaction->MetadataId), pTransaction->MetadataType);

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(2u + 1u + 2u * 2u, sub.numNotifications());
            ASSERT_EQ(1u, sub.numMatchingNotifications());

            const auto& notification = sub.matchingNotifications()[0];
            EXPECT_EQ(metadataId, notification.MetadataId);
            EXPECT_EQ(2u, notification.Modifications.size());
        }

        // endregion

        // region metadata field notification

        static void AssertCanPublishModifyMetadataFieldNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<model::ModifyMetadataV1FieldNotification<1>> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataV1ModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataV1ModificationType::Del, 3, 4).get()
            });

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(2u + 1u + 2u * 2u, sub.numNotifications());
            ASSERT_EQ(2u, sub.numMatchingNotifications());

            auto counter = 0u;
            for (const auto& modification : pTransaction->Transactions()) {
                const auto& notification = sub.matchingNotifications()[counter++];
                EXPECT_EQ(modification.ModificationType, notification.ModificationType);
                EXPECT_EQ(modification.KeySize, notification.KeySize);
                EXPECT_EQ_MEMORY(modification.KeyPtr(), notification.KeyPtr, modification.KeySize);
                EXPECT_EQ(modification.ValueSize, notification.ValueSize);
                EXPECT_EQ_MEMORY(modification.ValuePtr(), notification.ValuePtr, modification.ValueSize);
            }

            EXPECT_EQ(2u, counter);
        }

        // endregion

        // region metadata value notification

        static void AssertCanPublishModifyMetadataValueNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<typename TTraits::ModifyMetadataValueNotification> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataV1ModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataV1ModificationType::Del, 3, 4).get()
            });

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(2u + 1u + 2u * 2u, sub.numNotifications());
            ASSERT_EQ(2u, sub.numMatchingNotifications());

            auto counter = 0u;
            for (const auto& modification : pTransaction->Transactions()) {
                const auto& notification = sub.matchingNotifications()[counter++];
                EXPECT_EQ(pTransaction->MetadataId, notification.MetadataId);
                EXPECT_EQ(pTransaction->MetadataType, notification.MetadataType);
                EXPECT_EQ(modification.ModificationType, notification.ModificationType);
                EXPECT_EQ(modification.KeySize, notification.KeySize);
                EXPECT_EQ_MEMORY(modification.KeyPtr(), notification.KeyPtr, modification.KeySize);
                EXPECT_EQ(modification.ValueSize, notification.ValueSize);
                EXPECT_EQ_MEMORY(modification.ValuePtr(), notification.ValuePtr, modification.ValueSize);
            }

            EXPECT_EQ(2u, counter);
        }

        // endregion
    };

    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Address, _Address, model::Entity_Type_Address_Metadata)
    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Mosaic, _Mosaic, model::Entity_Type_Mosaic_Metadata)
    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Namespace, _Namespace, model::Entity_Type_Namespace_Metadata)

#define MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, TEST_NAME) \
	TEST(TEST_CLASS, TEST_NAME##_Regular##TEST_POSTFIX) { \
		MetadataTransactionPluginTests<TRAITS_PREFIX##RegularTraits>::Assert##TEST_NAME(); \
	} \
	TEST(TEST_CLASS, TEST_NAME##_Embedded##TEST_POSTFIX) { \
		MetadataTransactionPluginTests<TRAITS_PREFIX##EmbeddedTraits>::Assert##TEST_NAME(); \
	}

#define DEFINE_METADATA_TRANSACTION_PLUGIN_TESTS(TRAITS_PREFIX, TEST_POSTFIX) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanCalculateSize) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanPublishMetadataTypeNotification) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanPublishModifyMetadataNotification) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanPublishMetadataModificationsNotification) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanPublishModifyMetadataFieldNotification) \
	MAKE_METADATA_TRANSACTION_PLUGIN_TEST(TRAITS_PREFIX, TEST_POSTFIX, CanPublishModifyMetadataValueNotification)

    DEFINE_METADATA_TRANSACTION_PLUGIN_TESTS(Address, _Address)
    DEFINE_METADATA_TRANSACTION_PLUGIN_TESTS(Mosaic, _Mosaic)
    DEFINE_METADATA_TRANSACTION_PLUGIN_TESTS(Namespace, _Namespace)
}}
