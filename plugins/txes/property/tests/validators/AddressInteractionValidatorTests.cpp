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
#include "catapult/model/Address.h"
#include "tests/test/PropertyCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS AddressInteractionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AddressInteraction,)

	namespace {
		constexpr auto Default_Network = model::NetworkIdentifier::Zero;

		using CacheContents = std::unordered_map<Key, std::vector<Key>, utils::ArrayHasher<Key>>;

		struct AddressTraits {
			static model::AddressSet ParticipantsByAddress(const std::vector<Key>& keys) {
				model::AddressSet addresses;
				for (const auto& key : keys)
					addresses.insert(model::PublicKeyToAddress(key, Default_Network));

				return addresses;
			}

			static utils::KeySet ParticipantsByKey(const std::vector<Key>&) {
				return {};
			}
		};

		struct KeyTraits {
			static model::AddressSet ParticipantsByAddress(const std::vector<Key>&) {
				return {};
			}

			static utils::KeySet ParticipantsByKey(const std::vector<Key>& keys) {
				return utils::KeySet(keys.cbegin(), keys.cend());
			}
		};

		template<typename TOperationTraits>
		void PopulateCache(cache::CatapultCache& cache, const CacheContents& cacheContents) {
			auto delta = cache.createDelta();
			auto& propertyCacheDelta = delta.sub<cache::PropertyCache>();
			for (const auto& pair : cacheContents) {
				auto sourceAddress = model::PublicKeyToAddress(pair.first, Default_Network);
				propertyCacheDelta.insert(state::AccountProperties(sourceAddress));
				auto& accountProperties = propertyCacheDelta.find(sourceAddress).get();
				auto& accountProperty = accountProperties.property(model::PropertyType::Address);
				for (const auto& value : pair.second)
					TOperationTraits::Add(accountProperty, state::ToVector(model::PublicKeyToAddress(value, Default_Network)));
			}

			cache.commit(Height(1));
		}

		template<typename TOperationTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const CacheContents& cacheContents,
				const Key& source,
				const model::AddressSet& participantsByAddress,
				const utils::KeySet& participantsByKey) {
			// Arrange:
			auto cache = test::PropertyCacheFactory::Create();
			PopulateCache<TOperationTraits>(cache, cacheContents);
			auto pValidator = CreateAddressInteractionValidator();
			auto notification = model::AddressInteractionNotification(source, participantsByAddress, participantsByKey);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AddressTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Key) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<KeyTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region failure

	TRAITS_BASED_TEST(FailureWhenDestinationIsKnownAndSourceIsNotContainedInValues_Allow) {
		// Arrange:
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		auto values = test::GenerateRandomDataVector<Key>(3);
		CacheContents cacheContents{ { sourceKey, values } };

		// Act:
		AssertValidationResult<test::AllowTraits>(
				Failure_Property_Signer_Address_Interaction_Not_Allowed,
				cacheContents,
				test::GenerateRandomData<Key_Size>(),
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	TRAITS_BASED_TEST(FailureWhenDestinationIsKnownAndSourceIsContainedInValues_Block) {
		// Arrange:
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		auto values = test::GenerateRandomDataVector<Key>(3);
		CacheContents cacheContents{ { sourceKey, values } };

		// Act:
		AssertValidationResult<test::BlockTraits>(
				Failure_Property_Signer_Address_Interaction_Not_Allowed,
				cacheContents,
				values[1],
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	// endregion

	// region success

	namespace {
		template<typename TOperationTraits, typename TTraits>
		void AssertSuccessWhenDestinationAddressIsNotKnown() {
			// Arrange:
			auto sourceKey = test::GenerateRandomData<Key_Size>();
			auto values = test::GenerateRandomDataVector<Key>(3);
			CacheContents cacheContents{ { sourceKey, values } };

			// Act:
			AssertValidationResult<TOperationTraits>(
					ValidationResult::Success,
					cacheContents,
					values[1],
					TTraits::ParticipantsByAddress({ test::GenerateRandomData<Key_Size>() }),
					TTraits::ParticipantsByKey({ test::GenerateRandomData<Key_Size>() }));
		}
	}

	TRAITS_BASED_TEST(SuccessWhenDestinationAddressIsNotKnown_Allow) {
		// Assert: note that this situation will not occur during normal operation
		AssertSuccessWhenDestinationAddressIsNotKnown<test::AllowTraits, TTraits>();
	}

	TRAITS_BASED_TEST(SuccessWhenDestinationAddressIsNotKnown_Block) {
		// Assert:
		AssertSuccessWhenDestinationAddressIsNotKnown<test::BlockTraits, TTraits>();
	}

	namespace {
		template<typename TOperationTraits, typename TTraits>
		void AssertSuccessWhenValuesAreEmpty() {
			// Arrange:
			auto sourceKey = test::GenerateRandomData<Key_Size>();
			CacheContents cacheContents{ { sourceKey, {} } };

			// Act:
			AssertValidationResult<TOperationTraits>(
					ValidationResult::Success,
					cacheContents,
					test::GenerateRandomData<Key_Size>(),
					TTraits::ParticipantsByAddress({ sourceKey }),
					TTraits::ParticipantsByKey({ sourceKey }));
		}
	}

	TRAITS_BASED_TEST(SuccessWhenDestinationAddressIsKnownAndValuesAreEmpty_Allow) {
		// Assert:
		AssertSuccessWhenValuesAreEmpty<test::AllowTraits, TTraits>();
	}

	TRAITS_BASED_TEST(SuccessWhenDestinationAddressIsKnownAndValuesAreEmpty_Block) {
		// Assert:
		AssertSuccessWhenValuesAreEmpty<test::BlockTraits, TTraits>();
	}

	TRAITS_BASED_TEST(SuccessWhenSourceEqualsDestination_Allow) {
		// Arrange: source is not an allowed value
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		CacheContents cacheContents{ { sourceKey, {} } };

		// Act:
		AssertValidationResult<test::AllowTraits>(
				ValidationResult::Success,
				cacheContents,
				sourceKey,
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	TRAITS_BASED_TEST(SuccessWhenSourceEqualsDestination_Block) {
		// Arrange: source is a blocked value
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		CacheContents cacheContents{ { sourceKey, { sourceKey } } };

		// Act:
		AssertValidationResult<test::BlockTraits>(
				ValidationResult::Success,
				cacheContents,
				sourceKey,
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	TRAITS_BASED_TEST(SuccessWhenAllConditionsAreMet_Allow) {
		// Arrange:
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		auto values = test::GenerateRandomDataVector<Key>(3);
		CacheContents cacheContents{ { sourceKey, values } };

		// Act: destination address is known and source address is an allowed value
		AssertValidationResult<test::AllowTraits>(
				ValidationResult::Success,
				cacheContents,
				values[1],
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	TRAITS_BASED_TEST(SuccessWhenAllConditionsAreMet_Block) {
		// Arrange:
		auto sourceKey = test::GenerateRandomData<Key_Size>();
		auto values = test::GenerateRandomDataVector<Key>(3);
		CacheContents cacheContents{ { sourceKey, values } };

		// Act: destination address is known and source address is not a blocked value
		AssertValidationResult<test::BlockTraits>(
				ValidationResult::Success,
				cacheContents,
				test::GenerateRandomData<Key_Size>(),
				TTraits::ParticipantsByAddress({ sourceKey }),
				TTraits::ParticipantsByKey({ sourceKey }));
	}

	// endregion
}}
