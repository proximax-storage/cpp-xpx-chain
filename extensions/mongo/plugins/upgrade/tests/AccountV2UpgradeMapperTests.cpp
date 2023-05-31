/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/AccountV2UpgradeMapper.h"
#include "sdk/src/builders/AccountV2UpgradeBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS AccountV2UpgradeMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(AccountV2Upgrade)

		auto CreateAccountV2UpgradeTransactionBuilder(
				const Key& signer,
				const Key& newAccountKey) {
			builders::AccountV2UpgradeBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setNewAccount(newAccountKey);

			return builder;
		}

		template<typename TTraits>
		void AssertCanMapAccountV2UpgradeTransaction(
				const Key& newAccountKey) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pBuilder = CreateAccountV2UpgradeTransactionBuilder(signer, newAccountKey);
			auto pTransaction = TTraits::Adapt(pBuilder);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			EXPECT_EQ(pTransaction->NewAccountPublicKey, test::GetKeyValue(view, "newAccountPublicKey"));
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_AccountV2_Upgrade)

	// region streamTransaction

	PLUGIN_TEST(CanMapAccountV2UpgradeTransactionWithEmptyValues) {
		// Assert:
		AssertCanMapAccountV2UpgradeTransaction<TTraits>(Key());
	}

	PLUGIN_TEST(CanMapAccountV2UpgradeTransactionWithNonEmptyValues) {
		// Assert:
		Key key = test::GenerateRandomByteArray<Key>();
		AssertCanMapAccountV2UpgradeTransaction<TTraits>(key);
	}

	// endregion
}}}
