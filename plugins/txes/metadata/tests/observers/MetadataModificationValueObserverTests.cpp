/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/observers/Observers.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS MetadataModificationValueObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::MetadataCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(AddressMetadataValueModification,)
	DEFINE_COMMON_OBSERVER_TESTS(MosaicMetadataValueModification,)
	DEFINE_COMMON_OBSERVER_TESTS(NamespaceMetadataValueModification,)

	namespace {
		constexpr auto Add = model::MetadataModificationType::Add;
		constexpr auto Del = model::MetadataModificationType::Del;
		const Address Metadata_Main_Address = test::GenerateRandomByteArray<Address>();
		const MosaicId Metadata_Main_MosaicId = MosaicId(25);
		const NamespaceId Metadata_Main_NamespaceId = NamespaceId(25);
		const Height CurrentHeight = Height(2);

		struct AddressMetadataTraits {
			using ValueType = UnresolvedAddress;
			using Notification = model::ModifyAddressMetadataValueNotification_v1;

			static constexpr auto Metadata_Type = model::MetadataType::Address;

			static auto MainMetadataId() {
				return extensions::CopyToUnresolvedAddress(Metadata_Main_Address);
			}

			static constexpr auto CreateObserver = CreateAddressMetadataValueModificationObserver;
		};

		struct MosaicMetadataTraits {
			using ValueType = UnresolvedMosaicId;
			using Notification = model::ModifyMosaicMetadataValueNotification_v1;

			static constexpr auto Metadata_Type = model::MetadataType::MosaicId;

			static auto MainMetadataId() {
				return ValueType(Metadata_Main_MosaicId.unwrap());
			}

			static constexpr auto CreateObserver = CreateMosaicMetadataValueModificationObserver;
		};

		struct NamespaceMetadataTraits  {
			using ValueType = NamespaceId;
			using Notification = model::ModifyNamespaceMetadataValueNotification_v1;

			static constexpr auto Metadata_Type = model::MetadataType::NamespaceId;

			static auto MainMetadataId() {
				return Metadata_Main_NamespaceId;
			}

			static constexpr auto CreateObserver = CreateNamespaceMetadataValueModificationObserver;
		};


		template<typename TMetadataValueTraits>
		void PopulateCache(cache::CatapultCacheDelta& delta, const std::vector<state::MetadataField>& initValues) {
			auto& metadataCacheDelta = delta.sub<cache::MetadataCache>();
			auto metadataEntry = state::MetadataEntry(state::ToVector(TMetadataValueTraits::MainMetadataId()), TMetadataValueTraits::Metadata_Type);
			metadataEntry.fields().assign(initValues.begin(), initValues.end());

			metadataCacheDelta.insert(metadataEntry);
		}

		template<typename TMetadataValueTraits>
		void AssertCache(
				const cache::MetadataCacheDelta& delta,
				const typename TMetadataValueTraits::ValueType& metadataId,
				const std::vector<state::MetadataField>& expectedFields) {
			// Assert:

			auto id = state::GetHash(state::ToVector(metadataId), TMetadataValueTraits::Metadata_Type);

			auto iter = delta.find(id);
			if (expectedFields.empty()) {
				EXPECT_FALSE(iter.tryGet());
				return;
			}

			const auto& metadataEntry = iter.get();

			EXPECT_EQ(metadataEntry.fields().size(), expectedFields.size());
			for (auto i = 0u; i < metadataEntry.fields().size(); ++i) {
				EXPECT_EQ(metadataEntry.fields()[i], expectedFields[i]);
			}
		}

		struct Modification {
			std::string Key;
			std::string Value;
			model::MetadataModificationType Type;
		};

		template<typename TMetadataValueTraits>
		void RunTest(
				const std::vector<state::MetadataField>& initValues,
				Modification modification,
				const std::vector<state::MetadataField>& expectedFields,
				observers::NotifyMode notifyMode) {
			// Arrange:
			ObserverTestContext context(notifyMode, CurrentHeight);

			PopulateCache<TMetadataValueTraits>(context.cache(), initValues);

			auto notification = typename TMetadataValueTraits::Notification(
					TMetadataValueTraits::MainMetadataId(), TMetadataValueTraits::Metadata_Type,
					modification.Type,
					modification.Key.size(), modification.Key.data(),
					modification.Value.size(), modification.Value.data());

			auto pObserver = TMetadataValueTraits::CreateObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert:
			AssertCache<TMetadataValueTraits>(
					context.cache().sub<cache::MetadataCache>(),
					TMetadataValueTraits::MainMetadataId(),
					expectedFields);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TMetadataValueTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AddressMetadataTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Mosaic) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MosaicMetadataTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Namespace) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NamespaceMetadataTraits>(); } \
	template<typename TMetadataValueTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region commit

	TRAITS_BASED_TEST(ObserverAddMetadataToEmptyCache_Commit) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{},
				{ "Key", "Value", Add },
				{ { "Key", "Value", Height(0) } },
				NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(ObserverChangeValueOfMetadataField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value", Height(0) }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				{ "Key", "ValueNew", Add },
				{ { "Key", "Value", CurrentHeight }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) }, { "Key", "ValueNew", Height(0) } },
				NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(ObserverRemoveFieldFromMetadata) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value", Height(0) }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				{ "Key", "", Del },
				{ { "Key", "Value", CurrentHeight }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(ObserverRemoveFieldFromMetadata_WhenItContainsRedefinedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", Height(0) } },
				{ "Key", "", Del },
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", CurrentHeight } },
				 NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(ObserverAddField_WhenIsContainsRemovedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight - Height(1) } },
				{ "Key", "Value2", Add },
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", Height(0) } },
				NotifyMode::Commit);
	}

		TRAITS_BASED_TEST(ObserverAddsField_WhenFieldRemovedAtTheSameHeight) {
			// Act:
			RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", Height(0) }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				{ "Key", "Value3", Add },
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight }, { "Key", "Value3", Height(0) } },
				NotifyMode::Commit);
		}

		TRAITS_BASED_TEST(ObserverRemovesField_WhenFieldAddedAtTheSameHeight) {
			// Act:
			RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key", "Value3", Height(0) }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				{ "Key", "", Del },
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key", "Value3", CurrentHeight }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				NotifyMode::Commit);
		}

	// endregion

	// region rollback

	TRAITS_BASED_TEST(ObserverRevertAddMetadataToEmptyCache) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value", Height(0) } },
				{ "Key", "Value", Add },
				{},
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertChangeValueOfMetadataField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value", CurrentHeight }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) }, { "Key", "ValueNew", Height(0) } },
				{ "Key", "ValueNew", Add },
				{ { "Key", "Value", Height(0) }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertRemoveFieldFromMetadata) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value", CurrentHeight }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				{ "Key", "", Del },
				{ { "Key", "Value", Height(0) }, { "Key1", "Value", Height(0) }, { "Key2", "Value", Height(0) } },
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertRemoveFieldFromMetadata_WhenItContainsRedefinedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", CurrentHeight } },
				{ "Key", "", Del },
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", Height(0) } },
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertAddField_WhenIsContainsRemovedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight - Height(1) }, { "Key", "Value2", Height(0) } },
				{ "Key", "Value2", Add },
				{ { "Key", "Value1", CurrentHeight - Height(1) } },
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertsLastAddedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key", "Value3", Height(0) }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				{ "Key", "Value3", Add },
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", Height(0) }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(ObserverRevertsLastRemovedField) {
		// Act:
		RunTest<TMetadataValueTraits>(
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key", "Value3", CurrentHeight }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				{ "Key", "", Del },
				{ { "Key", "Value1", CurrentHeight }, { "Key", "Value2", CurrentHeight }, { "Key", "Value3", Height(0) }, { "Key1", "Value4", Height(0) }, { "Key2", "Value5", CurrentHeight } },
				NotifyMode::Rollback);
	}

	// endregion
}}
