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

#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "src/validators/Validators.h"
#include "src/plugins/ModifyMultisigAccountTransactionPlugin.h"
#include "catapult/model/TransactionPlugin.h"
#include "tests/test/MultisigCacheTestUtils.h"
#include "tests/test/MultisigTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/aggregate/src/config/AggregateConfiguration.h"

namespace catapult { namespace validators {

#define TEST_CLASS MultisigAggregateEligibleCosignersValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MultisigAggregateEligibleCosignersV1, model::TransactionRegistry())
	DEFINE_COMMON_VALIDATOR_TESTS(MultisigAggregateEligibleCosignersV2, model::TransactionRegistry())

	namespace {
		auto CreateTransactionRegistry() {
			// use a registry with mock and multilevel multisig transactions registered
			//   mock is used to test default behavior
			//   multilevel multisig is used to test transactions with custom approval requirements
			model::TransactionRegistry transactionRegistry;
			transactionRegistry.registerPlugin(mocks::CreateMockTransactionPlugin(mocks::PluginOptionFlags::Not_Top_Level));
			transactionRegistry.registerPlugin(plugins::CreateModifyMultisigAccountTransactionPlugin());
			return transactionRegistry;
		}

		template<typename TTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				const Key& signer,
				const std::vector<Key>& embeddedSigners,
				const std::vector<Key>& cosigners,
				bool newCosignersMustApprove,
				bool strictSigner) {
			
			// Arrange: setup transactions
			std::vector<uint8_t> txBuffer(sizeof(model::EmbeddedTransaction) * embeddedSigners.size());
			auto* pTransactions = reinterpret_cast<model::EmbeddedTransaction*>(txBuffer.data());
			for (auto i = 0u; i < embeddedSigners.size(); ++i) {
				auto& transaction = pTransactions[i];
                transaction.Type = mocks::MockTransaction::Entity_Type;
				transaction.Size = sizeof(model::EmbeddedTransaction);
				transaction.Signer = embeddedSigners[i];
			}

			// - setup cosignatures
			auto cosignatures = test::GenerateCosignaturesFromCosigners<typename TTraits::Descriptor>(cosigners);
			auto transactionRegistry = CreateTransactionRegistry();

			using Notification = typename TTraits::Notification;
			Notification notification(signer, embeddedSigners.size(), pTransactions, cosignatures.size(), cosignatures.data());
			auto pValidator = TTraits::CreateValidator(transactionRegistry);

			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::MultisigConfiguration::Uninitialized();
			pluginConfig.NewCosignersMustApprove = newCosignersMustApprove;
			config.Network.SetPluginConfiguration(pluginConfig);

			auto aggregateConfig = config::AggregateConfiguration::Uninitialized();
			aggregateConfig.StrictSigner = strictSigner;
			config.Network.SetPluginConfiguration(aggregateConfig);
			
			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config.ToConst());

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	// region unknown account / known non-multisig account

	namespace {

		struct V1TraitsMixin {
			using Notification = model::AggregateCosignaturesNotification<1>;
			using Descriptor = model::AggregateTransactionRawDescriptor;
			static auto CreateValidator(const model::TransactionRegistry& registry)
			{
				return CreateMultisigAggregateEligibleCosignersV1Validator(registry);
			}
		};

		struct V2TraitsMixin {
			using Descriptor = model::AggregateTransactionExtendedDescriptor;
			using Notification = model::AggregateCosignaturesNotification<3>;
			static auto CreateValidator(const model::TransactionRegistry& registry)
			{
				return CreateMultisigAggregateEligibleCosignersV2Validator(registry);
			}
		};

		template<typename TMixin>
		struct UnknownAccountTraits : public TMixin{
			static auto CreateCache(const Key&) {
				// return an empty cache
				return test::MultisigCacheFactory::Create();
			}
		};

		template<typename TMixin>
		struct CosignatoryAccountTraits : public TMixin{
			static auto CreateCache(const Key& aggregateSigner) {
				auto cache = test::MultisigCacheFactory::Create();
				auto cacheDelta = cache.createDelta();

				// make the aggregate signer a cosigner of a different account
				test::MakeMultisig(cacheDelta, test::GenerateRandomByteArray<Key>(), { aggregateSigner });

				cache.commit(Height());
				return cache;
			}
		};

		template<typename TTraits>
		static void AssertValidationResult(
				ValidationResult expectedResult,
				const Key& aggregateSigner,
				const std::vector<Key>& embeddedSigners,
				const std::vector<Key>& cosigners,
				bool newCosignersMustApprove,
				bool strictSigner) {
			// Arrange:
			auto cache = TTraits::CreateCache(aggregateSigner);

			// Assert:
			AssertValidationResult<TTraits>(expectedResult, cache, aggregateSigner, embeddedSigners, cosigners, newCosignersMustApprove, strictSigner);
		}
	}

#define NON_MULTISIG_TRAITS_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove); \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustApprove_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits<V1TraitsMixin>>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustNotApprove_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits<V1TraitsMixin>>(false); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustApprove_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits<V1TraitsMixin>>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustNotApprove_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits<V1TraitsMixin>>(false); } \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustApprove_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits<V2TraitsMixin>>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustNotApprove_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits<V2TraitsMixin>>(false); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustApprove_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits<V2TraitsMixin>>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustNotApprove_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits<V2TraitsMixin>>(false); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove)

	NON_MULTISIG_TRAITS_TEST(CosignerIsEligibleWhenItMatchesEmbeddedTransactionSigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);

		// Assert: valid because both aggregate signer and cosigner are also embedded transaction signers
		auto i = 0u;
		for (const auto& embeddedSigner : embeddedSigners) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<TTraits>(ValidationResult::Success, embeddedSigners[0], embeddedSigners, { embeddedSigner }, newCosignersMustApprove, true);
		}
	}

	NON_MULTISIG_TRAITS_TEST(CosignerIsIneligibleWhenItIsUnrelatedAccount) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto ineligibleSigner = test::GenerateRandomByteArray<Key>();

		// Assert: invalid because cosigner is not an embedded transaction signer
		AssertValidationResult<TTraits>(Failure_Aggregate_Ineligible_Cosigners, embeddedSigners[0], embeddedSigners, { ineligibleSigner }, newCosignersMustApprove, true);
	}

	NON_MULTISIG_TRAITS_TEST(AggregateSignerIsIneligibleWhenItIsUnrelatedAccount) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto ineligibleSigner = test::GenerateRandomByteArray<Key>();

		// Assert: invalid because aggregate signer is not an embedded transaction signer
		AssertValidationResult<TTraits>(Failure_Aggregate_Ineligible_Cosigners, ineligibleSigner, embeddedSigners, { embeddedSigners[1] }, newCosignersMustApprove, true);
	}

	NON_MULTISIG_TRAITS_TEST(CosignersAreEligibleWhenAllMatchEmbeddedTransactionSigners) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);

		// Assert: valid because aggregate signer and all cosigners are also embedded transaction signers
		AssertValidationResult<TTraits>(ValidationResult::Success, embeddedSigners[0], embeddedSigners, embeddedSigners, newCosignersMustApprove, true);
	}

	NON_MULTISIG_TRAITS_TEST(CosignersAreIneligibleWhenAnyDoesNotMatchEmbeddedTransactionSigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto cosigners = { embeddedSigners[0], test::GenerateRandomByteArray<Key>(), embeddedSigners[2] };

		// Assert: invalid because a single cosigner is not an embedded transaction signer
		AssertValidationResult<TTraits>(Failure_Aggregate_Ineligible_Cosigners, embeddedSigners[0], embeddedSigners, cosigners, newCosignersMustApprove, true);
	}
	
	NON_MULTISIG_TRAITS_TEST(AggregateSignerIsIneligibleButAllowInConfig) {
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto ineligibleSigner = test::GenerateRandomByteArray<Key>();
		
		// Assert: success because we allow ineligible signer
		AssertValidationResult<TTraits>(ValidationResult::Success, ineligibleSigner, embeddedSigners, { embeddedSigners[1] }, newCosignersMustApprove, false);
	}

	// endregion

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits>          \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove); \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustApproveV1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TraitsMixin>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustNotApproveV1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TraitsMixin>(false); } \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustApproveV2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TraitsMixin>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustNotApproveV2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TraitsMixin>(false); } \
    template<typename TTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove)

	// region multisig

	namespace {
		auto CreateCacheWithSingleLevelMultisig(const Key& embeddedSigner, const std::vector<Key>& cosignatories) {
			auto cache = test::MultisigCacheFactory::Create();
			auto cacheDelta = cache.createDelta();

			test::MakeMultisig(cacheDelta, embeddedSigner, cosignatories, 3, 4); // make a 3-4-X multisig

			cache.commit(Height());
			return cache;
		}
	}

	TRAITS_BASED_TEST(CosignerIsEligibleWhenItMatchesDirectEmbeddedTransactionSignerCosigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto embeddedCosignatories = test::GenerateRandomDataVector<Key>(4);
		auto cache = CreateCacheWithSingleLevelMultisig(embeddedSigners[1], embeddedCosignatories);

		// Assert: valid because embeddedCosignatory is a direct cosignatory of an embedded tx signer
		auto i = 0u;
		for (const auto& embeddedCosignatory : embeddedCosignatories) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<TTraits>(ValidationResult::Success, cache, embeddedSigners[0], embeddedSigners, { embeddedCosignatory }, newCosignersMustApprove, true);
		}
	}

	TRAITS_BASED_TEST(AggregateSignerIsEligibleWhenItMatchesDirectEmbeddedTransactionSignerCosigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto embeddedCosignatories = test::GenerateRandomDataVector<Key>(4);
		auto cache = CreateCacheWithSingleLevelMultisig(embeddedSigners[1], embeddedCosignatories);

		// Assert: valid because embeddedCosignatory is a direct cosignatory of an embedded tx signer
		auto i = 0u;
		for (const auto& embeddedCosignatory : embeddedCosignatories) {
			CATAPULT_LOG(debug) << "signing aggregate with " << i++;
			AssertValidationResult<TTraits>(ValidationResult::Success, cache, embeddedCosignatory, embeddedSigners, { embeddedSigners[0] }, newCosignersMustApprove, true);
		}
	}

	namespace {
		auto CreateCacheWithMultilevelMultisig(
				const Key& embeddedSigner,
				const std::vector<Key>& cosignatories,
				const std::vector<Key>& secondLevelCosignatories) {
			auto cache = test::MultisigCacheFactory::Create();
			auto cacheDelta = cache.createDelta();

			test::MakeMultisig(cacheDelta, embeddedSigner, cosignatories, 2, 3); // make a 2-3-X multisig
			test::MakeMultisig(cacheDelta, cosignatories[1], secondLevelCosignatories, 3, 4); // make a 3-4-X multisig

			cache.commit(Height());
			return cache;
		}
	}

	TRAITS_BASED_TEST(CosignerIsEligibleWhenItMatchesIndirectEmbeddedTransactionSignerCosigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto embeddedCosignatories = test::GenerateRandomDataVector<Key>(4);
		auto embeddedCosignatoriesL2 = test::GenerateRandomDataVector<Key>(5);
		auto cache = CreateCacheWithMultilevelMultisig(embeddedSigners[1], embeddedCosignatories, embeddedCosignatoriesL2);

		// Assert: valid because embeddedCosignatory is a second-level cosignatory of an embedded tx signer
		auto i = 0u;
		for (const auto& embeddedCosignatory : embeddedCosignatoriesL2) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<TTraits>(ValidationResult::Success, cache, embeddedSigners[0], embeddedSigners, { embeddedCosignatory }, newCosignersMustApprove, true);
		}
	}

	TRAITS_BASED_TEST(AggregateSignerIsEligibleWhenItMatchesIndirectEmbeddedTransactionSignerCosigner) {
		// Arrange:
		auto embeddedSigners = test::GenerateRandomDataVector<Key>(3);
		auto embeddedCosignatories = test::GenerateRandomDataVector<Key>(4);
		auto embeddedCosignatoriesL2 = test::GenerateRandomDataVector<Key>(5);
		auto cache = CreateCacheWithMultilevelMultisig(embeddedSigners[1], embeddedCosignatories, embeddedCosignatoriesL2);

		// Assert: valid because embeddedCosignatory is a second-level cosignatory of an embedded tx signer
		auto i = 0u;
		for (const auto& embeddedCosignatory : embeddedCosignatoriesL2) {
			CATAPULT_LOG(debug) << "signing aggregate with " << i++;
			AssertValidationResult<TTraits>(ValidationResult::Success, cache, embeddedCosignatory, embeddedSigners, { embeddedSigners[0] }, newCosignersMustApprove, true);
		}
	}

	// endregion

	// region multisig modify account handling

	namespace {
		constexpr auto Add = model::CosignatoryModificationType::Add;
		constexpr auto Del = model::CosignatoryModificationType::Del;

		template<typename TTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				const Key& signer,
				const model::EmbeddedTransaction& transaction,
				const std::vector<Key>& cosigners,
				bool newCosignersMustApprove,
				bool strictSigner) {
			// Arrange: setup cosignatures
			auto cosignatures = test::GenerateCosignaturesFromCosigners<typename TTraits::Descriptor>(cosigners);
			auto transactionRegistry = CreateTransactionRegistry();

			using Notification = typename TTraits::Notification;
			Notification notification(signer, 1, &transaction, cosignatures.size(), cosignatures.data());
			auto pValidator = TTraits::CreateValidator(transactionRegistry);

			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::MultisigConfiguration::Uninitialized();
			pluginConfig.NewCosignersMustApprove = newCosignersMustApprove;
			config.Network.SetPluginConfiguration(pluginConfig);
			
			auto aggregateConfig = config::AggregateConfiguration::Uninitialized();
			aggregateConfig.StrictSigner = strictSigner;
			config.Network.SetPluginConfiguration(aggregateConfig);
			
			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config.ToConst());

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, ModifyMultisigAccountAddedAccountsGainEligibilityWhenMustApproveV1) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = test::CreateModifyMultisigAccountTransaction(signer, { Add, Del, Add, Del });
		auto cache = test::MultisigCacheFactory::Create();

		// Assert: added accounts are eligible cosigners even though they aren't in the multisig cache
		auto i = 0u;
		const auto* pModifications = pTransaction->ModificationsPtr();
		for (const auto& cosignatory : { signer, pModifications[0].CosignatoryPublicKey, pModifications[2].CosignatoryPublicKey }) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<V1TraitsMixin>(ValidationResult::Success, cache, signer, *pTransaction, { cosignatory }, true, true);
		}
	}
	TEST(TEST_CLASS, ModifyMultisigAccountAddedAccountsGainEligibilityWhenMustApproveV2) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = test::CreateModifyMultisigAccountTransaction(signer, { Add, Del, Add, Del });
		auto cache = test::MultisigCacheFactory::Create();

		// Assert: added accounts are eligible cosigners even though they aren't in the multisig cache
		auto i = 0u;
		const auto* pModifications = pTransaction->ModificationsPtr();
		for (const auto& cosignatory : { signer, pModifications[0].CosignatoryPublicKey, pModifications[2].CosignatoryPublicKey }) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<V2TraitsMixin>(ValidationResult::Success, cache, signer, *pTransaction, { cosignatory }, true, true);
		}
	}

	TRAITS_BASED_TEST(ModifyMultisigAccountDeletedAccountsDoNotGainEligibility) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = test::CreateModifyMultisigAccountTransaction(signer, { Add, Del, Add, Del });
		auto cache = test::MultisigCacheFactory::Create();

		// Assert: deleted accounts do not have any special eligibility privileges
		auto i = 0u;
		const auto* pModifications = pTransaction->ModificationsPtr();
		for (const auto& cosignatory : { pModifications[1].CosignatoryPublicKey, pModifications[3].CosignatoryPublicKey }) {
			CATAPULT_LOG(debug) << "cosigning with cosigner " << i++;
			AssertValidationResult<TTraits>(Failure_Aggregate_Ineligible_Cosigners, cache, signer, *pTransaction, { cosignatory }, newCosignersMustApprove, true);
		}
	}

	TRAITS_BASED_TEST(MultisigAccountDeletedAccountsRetainExistingEligibility) {
		// Arrange: delete the (original eligible) embedded tx signer
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = test::CreateModifyMultisigAccountTransaction(signer, { Add, Del, Add, Del });
		pTransaction->ModificationsPtr()[1].CosignatoryPublicKey = signer;
		auto cache = test::MultisigCacheFactory::Create();

		// Assert: existing eligibility is retained
		//         (the embedded tx signer account is still allowed to sign even though it was deleted from a multisig account)
		AssertValidationResult<TTraits>(ValidationResult::Success, cache, signer, *pTransaction, { signer }, newCosignersMustApprove,true);
	}

	// endregion
}}
