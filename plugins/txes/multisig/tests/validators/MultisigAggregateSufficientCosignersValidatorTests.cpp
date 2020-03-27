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

#include "src/validators/Validators.h"
#include "src/plugins/ModifyMultisigAccountTransactionPlugin.h"
#include "catapult/model/TransactionPlugin.h"
#include "tests/test/MultisigCacheTestUtils.h"
#include "tests/test/MultisigTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MultisigAggregateSufficientCosignersValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(MultisigAggregateSufficientCosigners, model::TransactionRegistry())

	namespace {
		void AssertValidationResult(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				const Key& signer,
				const model::EmbeddedTransaction& subTransaction,
				const std::vector<Key>& cosigners,
				bool newCosignersMustApprove) {
			// Arrange: setup cosignatures
			auto cosignatures = test::GenerateCosignaturesFromCosigners(cosigners);

            // - use a registry with mock and multilevel multisig transactions registered
            //   mock is used to test default behavior
            //   multilevel multisig is used to test transactions with custom approval requirements
            model::TransactionRegistry transactionRegistry;
            transactionRegistry.registerPlugin(mocks::CreateMockTransactionPlugin(mocks::PluginOptionFlags::Not_Top_Level));
            transactionRegistry.registerPlugin(plugins::CreateModifyMultisigAccountTransactionPlugin());

            using Notification = model::AggregateEmbeddedTransactionNotification<1>;
			Notification notification(signer, subTransaction, cosignatures.size(), cosignatures.data());
			auto pValidator = CreateMultisigAggregateSufficientCosignersValidator(transactionRegistry);

			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::MultisigConfiguration::Uninitialized();
			pluginConfig.NewCosignersMustApprove = newCosignersMustApprove;
			config.Network.SetPluginConfiguration(pluginConfig);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config.ToConst());

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		auto CreateEmbeddedTransaction(const Key& signer) {
			auto pTransaction = std::make_unique<model::EmbeddedTransaction>();
			pTransaction->Type = mocks::MockTransaction::Entity_Type;
			pTransaction->Signer = signer;
			return pTransaction;
		}
	}

	// region unknown account / known non-multisig account

	namespace {
		struct UnknownAccountTraits {
			static auto CreateCache(const Key&) {
				// return an empty cache
				return test::MultisigCacheFactory::Create();
			}
		};

		struct CosignatoryAccountTraits {
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
				const Key& embeddedSigner,
				const Key& aggregateSigner,
				const std::vector<Key>& cosigners,
				bool newCosignersMustApprove) {
			// Arrange:
			auto cache = TTraits::CreateCache(embeddedSigner);
			auto pSubTransaction = CreateEmbeddedTransaction(embeddedSigner);

			// Assert:
			AssertValidationResult(expectedResult, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
		}
	}

#define NON_MULTISIG_TRAITS_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove); \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_UnknownAccount_NewCosignersMustNotApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<UnknownAccountTraits>(false); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits>(true); } \
	TEST(TEST_CLASS, TEST_NAME##_CosignatoryAccount_NewCosignersMustNotApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CosignatoryAccountTraits>(false); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove)

	NON_MULTISIG_TRAITS_TEST(SufficientWhenEmbeddedTransactionSignerIsAggregateSigner) {
		// Arrange:
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(2);

		// Assert: aggregate signer (implicit cosigner) is same as embedded transaction signer
		AssertValidationResult<TTraits>(ValidationResult::Success, aggregateSigner, aggregateSigner, cosigners, newCosignersMustApprove);
	}

	NON_MULTISIG_TRAITS_TEST(SufficientWhenEmbeddedTransactionSignerIsCosigner) {
		// Arrange:
		auto embeddedSigner = test::GenerateRandomByteArray<Key>();
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosigners = std::vector<Key>{ test::GenerateRandomByteArray<Key>(), embeddedSigner, test::GenerateRandomByteArray<Key>() };

		// Assert: one cosigner is same as embedded transaction signer
		AssertValidationResult<TTraits>(ValidationResult::Success, embeddedSigner, aggregateSigner, cosigners, newCosignersMustApprove);
	}

	NON_MULTISIG_TRAITS_TEST(InsufficientWhenTransactionSignerIsNeitherAggregateSignerNorCosigner) {
		// Arrange:
		auto embeddedSigner = test::GenerateRandomByteArray<Key>();
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosigners = test::GenerateRandomDataVector<Key>(2);

		// Assert: embedded transaction signer is neither aggregate signer nor cosigner
		AssertValidationResult<TTraits>(Failure_Aggregate_Missing_Cosigners, embeddedSigner, aggregateSigner, cosigners, newCosignersMustApprove);
	}

	// endregion

#define TRAITS_BASED_TEST(TEST_NAME) \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove); \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(true); } \
	TEST(TEST_CLASS, TEST_NAME##_NewCosignersMustNotApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(false); } \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(bool newCosignersMustApprove)

	// region single level multisig

	namespace {
		auto CreateCacheWithSingleLevelMultisig(
				const Key& embeddedSigner,
				const std::vector<Key>& cosignatories,
				uint8_t minApproval = 3,
				uint8_t minRemoval = 4) {
			auto cache = test::MultisigCacheFactory::Create();
			auto cacheDelta = cache.createDelta();

			test::MakeMultisig(cacheDelta, embeddedSigner, cosignatories, minApproval, minRemoval); // make a (3-4-X default) multisig

			cache.commit(Height());
			return cache;
		}

		template<typename TGetCosigners>
		void AssertBasicMultisigResult(ValidationResult expectedResult, TGetCosigners getCosigners, bool newCosignersMustApprove) {
			// Arrange:
			auto embeddedSigner = test::GenerateRandomByteArray<Key>();
			auto aggregateSigner = test::GenerateRandomByteArray<Key>();
			auto cosignatories = test::GenerateRandomDataVector<Key>(4);

			auto pSubTransaction = CreateEmbeddedTransaction(embeddedSigner);

			// - create the cache making the embedded signer a 3-4-4 multisig
			auto cache = CreateCacheWithSingleLevelMultisig(embeddedSigner, cosignatories);

			// Assert: 3 cosigners are required for approval
			auto cosigners = getCosigners(cosignatories);
			AssertValidationResult(expectedResult, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
		}
	}

	TRAITS_BASED_TEST(SufficientWhenMultisigEmbeddedTransactionSignerHasMinApprovers) {
		// Assert: 3 == 3
		AssertBasicMultisigResult(
				ValidationResult::Success,
				[](const auto& cosignatories) { return std::vector<Key>{ cosignatories[0], cosignatories[2], cosignatories[3] }; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SufficientWhenMultisigEmbeddedTransactionSignerHasGreaterThanMinApprovers) {
		// Assert: 4 > 3
		AssertBasicMultisigResult(
				ValidationResult::Success,
				[](const auto& cosignatories) { return cosignatories; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(InsufficientWhenMultisigEmbeddedTransactionSignerHasLessThanMinApprovers) {
		// Assert: 2 < 3
		AssertBasicMultisigResult(
				Failure_Aggregate_Missing_Cosigners,
				[](const auto& cosignatories) { return std::vector<Key>{ cosignatories[0], cosignatories[2] }; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SufficientWhenMultisigEmbeddedTransactionSignerHasMinApproversIncludingAggregateSigner) {
		// Arrange: include the aggregate signer as a cosignatory
		auto embeddedSigner = test::GenerateRandomByteArray<Key>();
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosignatories = test::GenerateRandomDataVector<Key>(4);
		cosignatories.push_back(aggregateSigner);

		auto pSubTransaction = CreateEmbeddedTransaction(embeddedSigner);

		// - create the cache making the embedded signer a 3-4-5 multisig
		auto cache = CreateCacheWithSingleLevelMultisig(embeddedSigner, cosignatories);

		// Assert: 1 (implicit) + 2 (explicit) == 3
		auto cosigners = std::vector<Key>{ cosignatories[2], cosignatories[1] };
		AssertValidationResult(ValidationResult::Success, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
	}

	// endregion

	// region multilevel multisig

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

		template<typename TGetCosigners>
		void AssertBasicMultilevelMultisigResult(ValidationResult expectedResult, TGetCosigners getCosigners, bool newCosignersMustApprove) {
			// Arrange:
			auto embeddedSigner = test::GenerateRandomByteArray<Key>();
			auto aggregateSigner = test::GenerateRandomByteArray<Key>();
			auto cosignatories = test::GenerateRandomDataVector<Key>(3);
			auto secondLevelCosignatories = test::GenerateRandomDataVector<Key>(4);

			auto pSubTransaction = CreateEmbeddedTransaction(embeddedSigner);

			// - create the cache making the embedded signer a 2-3-3 multisig where the second cosignatory is a 3-4-4 multisig
			auto cache = CreateCacheWithMultilevelMultisig(embeddedSigner, cosignatories, secondLevelCosignatories);

			// Assert: 2 first-level and 3 second-level cosigners are required for approval
			auto cosigners = getCosigners(secondLevelCosignatories);
			cosigners.push_back(cosignatories[1]);
			cosigners.push_back(cosignatories[2]);
			AssertValidationResult(expectedResult, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
		}
	}

	TRAITS_BASED_TEST(SufficientWhenMultilevelMultisigEmbeddedTransactionSignerHasMinApprovers) {
		// Assert: 3 == 3
		AssertBasicMultilevelMultisigResult(
				ValidationResult::Success,
				[](const auto& cosignatories) { return std::vector<Key>{ cosignatories[0], cosignatories[2], cosignatories[3] }; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SufficientWhenMultilevelMultisigEmbeddedTransactionSignerHasGreaterThanMinApprovers) {
		// Assert: 4 > 3
		AssertBasicMultilevelMultisigResult(
				ValidationResult::Success,
				[](const auto& cosignatories) { return cosignatories; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(InsufficientWhenMultilevelMultisigEmbeddedTransactionSignerHasLessThanMinApprovers) {
		// Assert: 2 < 3
		AssertBasicMultilevelMultisigResult(
				Failure_Aggregate_Missing_Cosigners,
				[](const auto& cosignatories) { return std::vector<Key>{ cosignatories[0], cosignatories[2] }; },
				newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SuccessWhenMultisigTransactionSignerHasMinApproversSharedAcrossLevels) {
		// Arrange:
		auto embeddedSigner = test::GenerateRandomByteArray<Key>();
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosignatories = test::GenerateRandomDataVector<Key>(3);
		auto secondLevelCosignatories = test::GenerateRandomDataVector<Key>(4);
		secondLevelCosignatories.push_back(cosignatories[2]);

		auto pSubTransaction = CreateEmbeddedTransaction(embeddedSigner);

		// - create the cache making the embedded signer a 2-3-3 multisig where the second cosignatory is a 3-4-5 multisig
		auto cache = CreateCacheWithMultilevelMultisig(embeddedSigner, cosignatories, secondLevelCosignatories);

		// Assert: 2 first-level and 3 second-level cosigners are required for approval (notice that cosignatories[2] cosigns both levels)
		auto cosigners = std::vector<Key>{
			cosignatories[1], cosignatories[2],
			secondLevelCosignatories[0], secondLevelCosignatories[1]
		};
		AssertValidationResult(ValidationResult::Success, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
	}

	// endregion

	// region multisig modify account handling

	namespace {
		constexpr auto Add = model::CosignatoryModificationType::Add;
		constexpr auto Del = model::CosignatoryModificationType::Del;

		void AddRequiredCosignersKeys(std::vector<Key>& cosigners, const model::EmbeddedModifyMultisigAccountTransaction& transaction) {
			auto numModifications = transaction.ModificationsCount;
			auto* pModification = transaction.ModificationsPtr();
			for (auto i = 0u; i < numModifications; ++i) {
				if (model::CosignatoryModificationType::Add == pModification->ModificationType)
					cosigners.push_back(pModification->CosignatoryPublicKey);

				++pModification;
			}
		}

		void AssertMinApprovalLimit(
				uint32_t expectedLimit,
				const std::vector<model::CosignatoryModificationType>& modificationTypes,
				uint8_t minApproval,
				uint8_t minRemoval,
				bool newCosignersMustApprove) {
			// Arrange:
			auto embeddedSigner = test::GenerateRandomByteArray<Key>();
			auto aggregateSigner = test::GenerateRandomByteArray<Key>();
			auto cosignatories = test::GenerateRandomDataVector<Key>(expectedLimit);

			auto pSubTransaction = test::CreateModifyMultisigAccountTransaction(embeddedSigner, modificationTypes);

			// - create the cache making the embedded signer a single level multisig
			auto cache = CreateCacheWithSingleLevelMultisig(embeddedSigner, cosignatories, minApproval, minRemoval);

			// Assert:
			CATAPULT_LOG(debug) << "running test with " << expectedLimit - 1 << " cosigners (insufficient)";
			auto insufficientCosigners = std::vector<Key>(cosignatories.cbegin(), cosignatories.cbegin() + expectedLimit - 1);
			AddRequiredCosignersKeys(insufficientCosigners, *pSubTransaction);
			AssertValidationResult(Failure_Aggregate_Missing_Cosigners, cache, aggregateSigner, *pSubTransaction, insufficientCosigners, newCosignersMustApprove);

			CATAPULT_LOG(debug) << "running test with " << expectedLimit << " cosigners (sufficient)";
			auto sufficientCosigners = std::vector<Key>(cosignatories.cbegin(), cosignatories.cbegin() + expectedLimit);
			AddRequiredCosignersKeys(sufficientCosigners, *pSubTransaction);
			AssertValidationResult(ValidationResult::Success, cache, aggregateSigner, *pSubTransaction, sufficientCosigners, newCosignersMustApprove);
		}
	}

	TRAITS_BASED_TEST(SingleAddModificationRequiresMinApproval) {
		// Assert:
		AssertMinApprovalLimit(3, { Add }, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(MultipleAddModificationsRequireMinApproval) {
		// Assert:
		AssertMinApprovalLimit(3, { Add, Add, Add }, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SingleDelModificationRequiresMinRemoval) {
		// Assert:
		AssertMinApprovalLimit(4, { Del }, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(MixedDelAndAddModificationsRequireMinRemovalWhenMinRemovalIsGreater) {
		// Assert:
		AssertMinApprovalLimit(4, { Add, Del, Add }, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(MixedDelAndAddModificationsRequireMinApprovalWhenMinApprovalIsGreater) {
		// Assert:
		AssertMinApprovalLimit(4, { Add, Del, Add }, 4, 3, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SingleUnknownModificationRequiresMinApproval) {
		// Assert:
		AssertMinApprovalLimit(3, { static_cast<model::CosignatoryModificationType>(0xCC) }, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(NonCosignatoryModificationRequiresMinApproval) {
		// Assert:
		AssertMinApprovalLimit(3, {}, 3, 4, newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(MinRemovalLimitIsAppliedAcrossMultipleLevels) {
		// Arrange:
		auto embeddedSigner = test::GenerateRandomByteArray<Key>();
		auto aggregateSigner = test::GenerateRandomByteArray<Key>();
		auto cosignatories = test::GenerateRandomDataVector<Key>(3);
		auto secondLevelCosignatories = test::GenerateRandomDataVector<Key>(4);
		auto base23Cosigners = std::vector<Key>{
			cosignatories[0], cosignatories[1],
			secondLevelCosignatories[0], secondLevelCosignatories[1], secondLevelCosignatories[2]
		};

		auto pSubTransaction = test::CreateModifyMultisigAccountTransaction(embeddedSigner, { Del });

		// - create the cache making the embedded signer a 2-3-3 multisig where the second cosignatory is a 3-4-4 multisig
		auto cache = CreateCacheWithMultilevelMultisig(embeddedSigner, cosignatories, secondLevelCosignatories);

		// Assert: 3 first-level and 4 second-level cosigners are required for approval
		CATAPULT_LOG(debug) << "running test with 3 + 3 cosigners (insufficient)";
		auto insufficient33Cosigners = base23Cosigners;
		insufficient33Cosigners.push_back(cosignatories[2]);
		AssertValidationResult(Failure_Aggregate_Missing_Cosigners, cache, aggregateSigner, *pSubTransaction, insufficient33Cosigners, newCosignersMustApprove);

		CATAPULT_LOG(debug) << "running test with 2 + 4 cosigners (insufficient)";
		auto insufficient24Cosigners = base23Cosigners;
		insufficient24Cosigners.push_back(secondLevelCosignatories[3]);
		AssertValidationResult(Failure_Aggregate_Missing_Cosigners, cache, aggregateSigner, *pSubTransaction, insufficient24Cosigners, newCosignersMustApprove);

		CATAPULT_LOG(debug) << "running test with 3 + 4 cosigners (sufficient)";
		auto sufficientCosigners = base23Cosigners;
		sufficientCosigners.push_back(cosignatories[2]);
		sufficientCosigners.push_back(secondLevelCosignatories[3]);
		AssertValidationResult(ValidationResult::Success, cache, aggregateSigner, *pSubTransaction, sufficientCosigners, newCosignersMustApprove);
	}

	// endregion

	// region cosignatory approval - utils

	namespace {
		enum class AccountPolicy { Regular, Multisig };

		void AddSingleLevelMultisig(cache::CatapultCache& cache, const Key& multisigPublicKey, const std::vector<Key>& cosignatories) {
			auto cacheDelta = cache.createDelta();
			test::MakeMultisig(cacheDelta, multisigPublicKey, cosignatories, 3, 3); // make a (3-3-X default) multisig
			cache.commit(Height());
		}

		void AddAll(std::vector<Key>& allCosigners, const std::vector<Key>& cosigners) {
			allCosigners.insert(allCosigners.cend(), cosigners.cbegin(), cosigners.cend());
		}

		template<typename TMergeKeys>
		void AssertCosignatoriesMustApproveTransaction(
				ValidationResult expectedResult,
				const std::vector<model::CosignatoryModificationType>& modificationTypes,
				const std::vector<AccountPolicy>& accountPolicies,
				TMergeKeys mergeKeys,
				bool newCosignersMustApprove) {
			// Sanity:
			ASSERT_EQ(modificationTypes.size(), accountPolicies.size());

			// Arrange:
			auto embeddedSigner = test::GenerateRandomByteArray<Key>();
			auto aggregateSigner = test::GenerateRandomByteArray<Key>();
			auto embeddedSignerCosignatories = test::GenerateRandomDataVector<Key>(3);

			auto pSubTransaction = test::CreateModifyMultisigAccountTransaction(embeddedSigner, modificationTypes);

			// - create the cache making the embedded signer a single level multisig
			auto cache = test::MultisigCacheFactory::Create();
			AddSingleLevelMultisig(cache, embeddedSigner, embeddedSignerCosignatories);

			// - make added cosigners single level multisig according to the multisig policies
			std::vector<Key> requiredCosignatories;
			auto numModifications = pSubTransaction->ModificationsCount;
			auto* pModification = pSubTransaction->ModificationsPtr();
			for (auto i = 0u; i < numModifications; ++i, ++pModification) {
				auto isAdded = model::CosignatoryModificationType::Add == pModification->ModificationType;
				if (isAdded && AccountPolicy::Multisig == accountPolicies[i] && newCosignersMustApprove) {
					// - CosignatoryPublicKey is not a required cosigner because it is a multisig account
					auto cosignerCosignatories = test::GenerateRandomDataVector<Key>(3);
					AddSingleLevelMultisig(cache, pModification->CosignatoryPublicKey, cosignerCosignatories);
					AddAll(requiredCosignatories, cosignerCosignatories);
				} else if (!isAdded || newCosignersMustApprove) {
					requiredCosignatories.push_back(pModification->CosignatoryPublicKey);
				}
			}

			// Assert:
			auto cosigners = mergeKeys(embeddedSignerCosignatories, requiredCosignatories);
			AssertValidationResult(expectedResult, cache, aggregateSigner, *pSubTransaction, cosigners, newCosignersMustApprove);
		}

		template<typename TTraits>
		void AssertCosignatoriesMustApproveTransaction(const std::vector<AccountPolicy>& accountPolicies) {
			// Assert:
			AssertCosignatoriesMustApproveTransaction(TTraits::ExpectedResult, { Add, Add, Add }, accountPolicies, TTraits::MergeKeys, TTraits::NewCosignersMustApprove);
		}

		struct ValidationFailureTraits {
			static constexpr auto ExpectedResult = Failure_Aggregate_Missing_Cosigners;
			static constexpr auto NewCosignersMustApprove = true;

			static auto MergeKeys(const std::vector<Key>& embeddedSignerCosignatories, const std::vector<Key>&) {
				return embeddedSignerCosignatories;
			}
		};

		struct ValidationSuccessTraits {
			static constexpr auto ExpectedResult = ValidationResult::Success;
			static constexpr auto NewCosignersMustApprove = true;

			static auto MergeKeys(const std::vector<Key>& embeddedSignerCosignatories, const std::vector<Key>& requiredCosignatories) {
				auto cosigners = std::vector<Key>(embeddedSignerCosignatories.cbegin(), embeddedSignerCosignatories.cend());
				AddAll(cosigners, requiredCosignatories);
				return cosigners;
			}
		};

		struct ValidationSuccessNewCosignersMustNotApproveTraits {
			static constexpr auto ExpectedResult = ValidationResult::Success;
			static constexpr auto NewCosignersMustApprove = false;

			static auto MergeKeys(const std::vector<Key>& embeddedSignerCosignatories, const std::vector<Key>&) {
				return embeddedSignerCosignatories;
			}
		};
	}

	// endregion

	// region cosignatory approval - shared

#define COSIGNER_APPROVAL_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Failure) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidationFailureTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Success) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidationSuccessTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Success_NewCosignersMustNotApprove) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidationSuccessNewCosignersMustNotApproveTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_SingleAdd_NotMultisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction(TTraits::ExpectedResult, { Add }, { AccountPolicy::Regular }, TTraits::MergeKeys, TTraits::NewCosignersMustApprove);
	}

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_SingleAdd_Multisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction(TTraits::ExpectedResult, { Add }, { AccountPolicy::Multisig }, TTraits::MergeKeys, TTraits::NewCosignersMustApprove);
	}

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_MultipleAdds_NoneMultisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction<TTraits>({ AccountPolicy::Regular, AccountPolicy::Regular, AccountPolicy::Regular });
	}

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_MultipleAdds_SomeMultisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction<TTraits>({ AccountPolicy::Multisig, AccountPolicy::Regular, AccountPolicy::Multisig });
	}

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_MultipleAdds_AllMultisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction<TTraits>({ AccountPolicy::Multisig, AccountPolicy::Multisig, AccountPolicy::Multisig });
	}

	COSIGNER_APPROVAL_TEST(CosignatoryApproval_AddsAndDelete_SomeMultisig) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction(
				TTraits::ExpectedResult,
				{ Add, Del, Add },
				{ AccountPolicy::Multisig, AccountPolicy::Multisig, AccountPolicy::Regular },
				TTraits::MergeKeys,
				TTraits::NewCosignersMustApprove);
	}

	// endregion

	// region cosignatory approval - failure

	TEST(TEST_CLASS, InsuffientWhenOnlySomeCosignatoriesApprove) {
		// Assert:
		AssertCosignatoriesMustApproveTransaction(
				Failure_Aggregate_Missing_Cosigners,
				{ Add, Del, Add },
				{ AccountPolicy::Multisig, AccountPolicy::Multisig, AccountPolicy::Regular },
				[](const auto& embeddedSignerCosignatories, const auto& requiredCosignatories) {
					auto cosigners = ValidationSuccessTraits::MergeKeys(embeddedSignerCosignatories, requiredCosignatories);
					cosigners.erase(--cosigners.cend());
					return cosigners;
				},
				true);
	}

	// endregion

	// region cosignatory approval - success

	TRAITS_BASED_TEST(SufficientWhenOnlyDeleteModification_NotMultisig) {
		// Assert:
		constexpr auto Success = ValidationResult::Success;
		AssertCosignatoriesMustApproveTransaction(Success, { Del }, { AccountPolicy::Regular }, [](const auto& cosigners, const auto&) {
			return cosigners;
		},
		newCosignersMustApprove);
	}

	TRAITS_BASED_TEST(SufficientWhenOnlyDeleteModification_Multisig) {
		// Assert:
		constexpr auto Success = ValidationResult::Success;
		AssertCosignatoriesMustApproveTransaction(Success, { Del }, { AccountPolicy::Multisig }, [](const auto& cosigners, const auto&) {
			return cosigners;
		},
		newCosignersMustApprove);
	}

	// endregion
}}
