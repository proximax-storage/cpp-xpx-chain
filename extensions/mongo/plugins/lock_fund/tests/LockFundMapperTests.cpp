/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/builders/LockFundCancelUnlockBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "sdk/src/builders/LockFundTransferBuilder.h"
#include "src/LockFundMapper.h"
#include "plugins/txes/lock_fund/src/model/LockFundTransferTransaction.h"
#include "plugins/txes/lock_fund/src/model/LockFundCancelUnlockTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS LockFundMapperTests

	namespace {

		constexpr UnresolvedMosaicId Test_Mosaic_Id(1234);

		auto CreateLockFundTransferTransactionBuilder(
				const Key& signer,
				const BlockDuration duration,
				const model::LockFundAction action,
				std::initializer_list<model::UnresolvedMosaic> mosaics) {
			builders::LockFundTransferBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setDuration(duration);
			builder.setAction(action);

			for (const auto& mosaic : mosaics)
				builder.addMosaic(mosaic);

			return builder;
		}

		template<typename TTransaction>
		void AssertEqualLockFundTransfer(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.Duration.unwrap(), test::GetUint64(dbTransaction, "duration"));
			EXPECT_EQ((uint8_t)transaction.Action, test::GetUint8(dbTransaction, "action"));
			auto dbMosaics = dbTransaction["mosaics"].get_array().value;
			ASSERT_EQ(transaction.MosaicsCount, test::GetFieldCount(dbMosaics));
			const auto* pMosaic = transaction.MosaicsPtr();
			auto iter = dbMosaics.cbegin();
			for (auto i = 0u; i < transaction.MosaicsCount; ++i) {
				EXPECT_EQ(pMosaic->MosaicId, UnresolvedMosaicId(test::GetUint64(iter->get_document().view(), "id")));
				EXPECT_EQ(pMosaic->Amount, Amount(test::GetUint64(iter->get_document().view(), "amount")));
				++pMosaic;
				++iter;
			}
		}
		template<typename TTraits>
		void AssertCanMapLockFundTransferTransaction(std::initializer_list<model::UnresolvedMosaic> mosaics) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pTransaction = TTraits::Adapt(CreateLockFundTransferTransactionBuilder(signer, BlockDuration(10), model::LockFundAction::Unlock, mosaics));
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(3u, test::GetFieldCount(view));
			AssertEqualLockFundTransfer(*pTransaction, view);
		}

		auto CreateLockFundCancelUnlockTransactionBuilder(
				const Key& signer,
				const Height TargetHeight) {
			builders::LockFundCancelUnlockBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setHeight(Height(100));
			return builder;
		}

		template<typename TTransaction>
		static void AssertLockFundCancelUnlockTransaction(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(dbTransaction["targetHeight"].get_int64().value, (int64_t)transaction.TargetHeight.unwrap());
		}

		template<typename TTraits>
		static void AssertCanMapLockFundCancelTransaction(Height targetHeight) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pTransaction = TTraits::Adapt(CreateLockFundCancelUnlockTransactionBuilder(signer, targetHeight));
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			AssertLockFundCancelUnlockTransaction(*pTransaction, view);
		}
	}

	namespace Transfer {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(LockFundTransfer)
		DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_Lock_Fund_Transfer)

		PLUGIN_TEST(CanMapLockFundTransferTransactionWithoutMosaics) {
			// Assert:
			AssertCanMapLockFundTransferTransaction<TTraits>({});
		}

		PLUGIN_TEST(CanMapLockFundTransferTransactionWithSingleMosaic) {
			// Assert:
			AssertCanMapLockFundTransferTransaction<TTraits>({ { Test_Mosaic_Id, Amount(234) } });
		}

		PLUGIN_TEST(CanMapLockFundTransferTransactionWithMultipleMosaics) {
			// Assert:
			AssertCanMapLockFundTransferTransaction<TTraits>(
					{ { Test_Mosaic_Id, Amount(234) }, { UnresolvedMosaicId(1357), Amount(345) }, { UnresolvedMosaicId(31), Amount(45) } });
		}
	}
	namespace CancelUnlock {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(LockFundCancelUnlock)
		DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_Lock_Fund_Cancel_Unlock)

		PLUGIN_TEST(CanMapLockFundCancelUnlockTransaction) {
			// Assert:
			AssertCanMapLockFundCancelTransaction<TTraits>(Height(10));
			AssertCanMapLockFundCancelTransaction<TTraits>(Height(550));
			AssertCanMapLockFundCancelTransaction<TTraits>(Height(123));
		}

	}




	// region streamTransaction


}}}
