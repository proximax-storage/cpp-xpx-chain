/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/model/AddressMetadataTransaction.h"
#include "src/model/MetadataNotifications.h"
#include "src/model/MosaicMetadataTransaction.h"
#include "src/model/NamespaceMetadataTransaction.h"
#include "src/plugins/MetadataTransactionPlugin.h"
#include "src/state/MetadataUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/MetadataTestUtils.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

#define TEST_CLASS MetadataTransactionPluginTests

	namespace {
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS_WITH_PREFIXED_TRAITS(AddressMetadata, 1, 1, AddressMetadata)
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS_WITH_PREFIXED_TRAITS(MosaicMetadata, 1, 1, MosaicMetadata)
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS_WITH_PREFIXED_TRAITS(NamespaceMetadata, 1, 1, NamespaceMetadata)

        template<typename TTransaction, typename TTransactionTraits>
        struct AddressTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyAddressMetadataNotification;
            using ModifyMetadataValueNotification = model::ModifyAddressMetadataValueNotification;
        };

        using AddressRegularTraits = AddressTraits<model::AddressMetadataTransaction, AddressMetadataRegularTraits>;
        using AddressEmbeddedTraits = AddressTraits<model::EmbeddedAddressMetadataTransaction, AddressMetadataEmbeddedTraits>;

        template<typename TTransaction, typename TTransactionTraits>
        struct MosaicTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyMosaicMetadataNotification;
            using ModifyMetadataValueNotification = model::ModifyMosaicMetadataValueNotification;
        };

        using MosaicRegularTraits = MosaicTraits<model::MosaicMetadataTransaction, MosaicMetadataRegularTraits>;
        using MosaicEmbeddedTraits = MosaicTraits<model::EmbeddedMosaicMetadataTransaction, MosaicMetadataEmbeddedTraits>;

        template<typename TTransaction, typename TTransactionTraits>
        struct NamespaceTraits : public TTransactionTraits {
            using TransactionType = TTransaction;
            using ModifyMetadataNotification = model::ModifyNamespaceMetadataNotification;
            using ModifyMetadataValueNotification = model::ModifyNamespaceMetadataValueNotification;
        };

        using NamespaceRegularTraits = NamespaceTraits<model::NamespaceMetadataTransaction, NamespaceMetadataRegularTraits>;
        using NamespaceEmbeddedTraits = NamespaceTraits<model::EmbeddedNamespaceMetadataTransaction, NamespaceMetadataEmbeddedTraits>;
	}

	template<typename TTraits>
	class MetadataTransactionPluginTests {
	public:
        // region TransactionPlugin

        static void AssertCanCalculateSize() {
            // Arrange:
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataModificationType::Del, 3, 4).get()
            });

            // Act:
            auto realSize = pPlugin->calculateRealSize(*pTransaction);

            // Assert:
            EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 2 * sizeof(model::MetadataModification) + 1 + 2 + 3 + 4, realSize);
        }

        // endregion

        // region metadata type notification

        static void AssertCanPublishMetadataTypeNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<model::MetadataTypeNotification> sub;
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
            mocks::MockTypedNotificationSubscriber<model::MetadataModificationsNotification> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataModificationType::Del, 3, 4).get()
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
            mocks::MockTypedNotificationSubscriber<model::ModifyMetadataFieldNotification> sub;
            auto pPlugin = TTraits::CreatePlugin();

            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>({
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataModificationType::Del, 3, 4).get()
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
                EXPECT_EQ(modification.KeyPtr(), notification.KeyPtr);
                EXPECT_EQ(modification.ValueSize, notification.ValueSize);
                EXPECT_EQ(modification.ValuePtr(), notification.ValuePtr);
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
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataModificationType::Del, 3, 4).get()
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
                EXPECT_EQ(modification.KeyPtr(), notification.KeyPtr);
                EXPECT_EQ(modification.ValueSize, notification.ValueSize);
                EXPECT_EQ(modification.ValuePtr(), notification.ValuePtr);
            }

            EXPECT_EQ(2u, counter);
        }

        // endregion
	};

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_WITH_PREFIXED_TRAITS(TEST_CLASS, Address, _Address, model::Entity_Type_Address_Metadata)
	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_WITH_PREFIXED_TRAITS(TEST_CLASS, Mosaic, _Mosaic, model::Entity_Type_Mosaic_Metadata)
	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_WITH_PREFIXED_TRAITS(TEST_CLASS, Namespace, _Namespace, model::Entity_Type_Namespace_Metadata)

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
