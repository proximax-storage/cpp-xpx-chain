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
#include "tests/test/MultisigCacheTestUtils.h"
#include "tests/test/MultisigTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyMultisigInvalidCosignersValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyMultisigInvalidCosigners,)

	namespace {
		using Modifications = std::vector<model::CosignatoryModification>;

		auto CreateNotification(const Key& signer, const std::vector<model::CosignatoryModification>& modifications) {
			return model::ModifyMultisigCosignersNotification<1>(signer, static_cast<uint8_t>(modifications.size()), modifications.data());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				Height height,
				const model::ModifyMultisigCosignersNotification<1>& notification) {
			// Arrange:
			auto pValidator = CreateModifyMultisigInvalidCosignersValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		constexpr auto Add = model::CosignatoryModificationType::Add;
		constexpr auto Del = model::CosignatoryModificationType::Del;

		void AssertMultisigAccountIsUnknown(ValidationResult expectedResult, model::CosignatoryModificationType operation) {
			// Arrange:
			auto keys = test::GenerateKeys(4);
			const auto& signer = keys[0];
			auto modifications = Modifications{ { Add, keys[1] }, { operation, keys[2] }, { Add, keys[3] } };
			auto notification = CreateNotification(signer, modifications);

			auto cache = test::MultisigCacheFactory::Create();

			// Assert:
			AssertValidationResult(expectedResult, cache, Height(100), notification);
		}
	}

	TEST(TEST_CLASS, CanAddCosignatoriesWhenAccountIsUnknown) {
		// Assert:
		AssertMultisigAccountIsUnknown(ValidationResult::Success, Add);
	}

	TEST(TEST_CLASS, CannotRemoveCosignatoryWhenAccountIsUnknown) {
		// Assert:
		AssertMultisigAccountIsUnknown(Failure_Multisig_Modify_Unknown_Multisig_Account, Del);
	}

	namespace {
		enum class CosignerType { Existing, New };

		struct OperationAndType {
		public:
			model::CosignatoryModificationType Operation;
			CosignerType Type;
		};

		void AssertCosignatoriesModifications(ValidationResult expectedResult, const std::vector<OperationAndType>& settings) {
			// Arrange: first key is a signer (multisig account key)
			auto keys = test::GenerateKeys(1 + settings.size());
			const auto& signer = keys[0];
			Modifications modifications;
			for (auto i = 0u; i < settings.size(); ++i)
				modifications.push_back({ settings[i].Operation, keys[1 + i] });

			auto notification = CreateNotification(signer, modifications);
			auto cache = test::MultisigCacheFactory::Create();

			// - create multisig entry in cache
			{
				auto delta = cache.createDelta();
				auto& multisigDelta = delta.sub<cache::MultisigCache>();
				const auto& multisigAccountKey = keys[0];
				multisigDelta.insert(state::MultisigEntry(multisigAccountKey));
				auto& cosignatories = multisigDelta.find(multisigAccountKey).get().cosignatories();
				for (auto i = 0u; i < settings.size(); ++i) {
					if (CosignerType::Existing == settings[i].Type)
						cosignatories.insert(keys[1 + i]);
				}

				cache.commit(Height(1));
			}

			// Assert:
			AssertValidationResult(expectedResult, cache, Height(100), notification);
		}
	}

	// region single

	TEST(TEST_CLASS, CanAddCosignatoryWhenNotPresent) {
		// Assert:
		AssertCosignatoriesModifications(ValidationResult::Success, { { Add, CosignerType::New } });
	}

	TEST(TEST_CLASS, CannotAddExistingCosignatory) {
		// Assert:
		AssertCosignatoriesModifications(Failure_Multisig_Modify_Already_A_Cosigner, { { Add, CosignerType::Existing } });
	}

	TEST(TEST_CLASS, CanRemoveExistingCosignatory) {
		// Assert:
		AssertCosignatoriesModifications(ValidationResult::Success, { { Del, CosignerType::Existing } });
	}

	TEST(TEST_CLASS, CannotRemoveCosignatoryWhenNotPresent) {
		// Assert:
		AssertCosignatoriesModifications(Failure_Multisig_Modify_Not_A_Cosigner, { { Del, CosignerType::New } });
	}

	// endregion

	// region multiple success

	TEST(TEST_CLASS, CanAddCosignatoriesWhenNotPresent) {
		// Assert:
		AssertCosignatoriesModifications(ValidationResult::Success, {
				{ Add, CosignerType::New },
				{ Add, CosignerType::New },
				{ Add, CosignerType::New }
		});
	}

	TEST(TEST_CLASS, CanRemoveExistingCosignatories) {
		// Assert: note that stateless validator will reject multiple deletions
		AssertCosignatoriesModifications(ValidationResult::Success, {
				{ Del, CosignerType::Existing },
				{ Del, CosignerType::Existing },
				{ Del, CosignerType::Existing }
		});
	}

	TEST(TEST_CLASS, CanAddNewAndRemoveExistingCosignatories) {
		// Assert:
		AssertCosignatoriesModifications(ValidationResult::Success, {
				{ Add, CosignerType::New },
				{ Del, CosignerType::Existing },
				{ Add, CosignerType::New },
				{ Del, CosignerType::Existing }
		});
	}

	// endregion

	// region multiple successes, single failure

	TEST(TEST_CLASS, CannotAddExistingCosignatory_Multiple) {
		// Assert:
		AssertCosignatoriesModifications(Failure_Multisig_Modify_Already_A_Cosigner, {
				{ Add, CosignerType::New },
				{ Add, CosignerType::Existing },
				{ Add, CosignerType::New },
		});
	}

	TEST(TEST_CLASS, CannotRemoveCosignatoryWhenNotPresent_Multiple) {
		// Assert:
		AssertCosignatoriesModifications(Failure_Multisig_Modify_Not_A_Cosigner, {
				{ Del, CosignerType::Existing },
				{ Del, CosignerType::New },
				{ Del, CosignerType::Existing }
		});
	}

	// endregion

	// region multiple failures

	TEST(TEST_CLASS, FirstErrorIsReported) {
		// Assert:
		AssertCosignatoriesModifications(Failure_Multisig_Modify_Already_A_Cosigner, {
				{ Add, CosignerType::Existing },
				{ Del, CosignerType::New }
		});

		AssertCosignatoriesModifications(Failure_Multisig_Modify_Not_A_Cosigner, {
				{ Del, CosignerType::New },
				{ Add, CosignerType::Existing }
		});
	}

	// endregion
}}
