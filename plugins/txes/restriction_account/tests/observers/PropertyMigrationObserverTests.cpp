/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "tests/test/core/NotificationTestUtils.h"
#include "plugins/txes/property/src/cache/PropertyCache.h"
#include "src/observers/Observers.h"
#include "tests/test/AccountRestrictionCacheTestUtils.h"
#include "plugins/txes/property/tests/test/PropertyCacheTestUtils.h"
#include "plugins/txes/property/tests/test/PropertyTestUtils.h"
#include "plugins/txes/property/tests/test/AccountPropertiesTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PropertyMigrationObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::AccountRestrictionCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(PropertyMigration,)

	namespace {

		constexpr auto Add = model::AccountRestrictionModificationAction::Add;
		constexpr auto Del = model::AccountRestrictionModificationAction::Del;
		constexpr auto Num_Default_Entries = 5u;

		template<typename TPropertyValueTraits>
		using EntryType = std::pair<Key, std::vector<typename TPropertyValueTraits::ValueType>>;

		template<typename TKind, typename = void>
		struct KindDecoder;

		template<typename TKind>
		struct KindDecoder<TKind, typename std::enable_if<!std::is_member_pointer<decltype(&TKind::Property_Type)>::value>::type>{
			using type = model::PropertyType;
			using ValueType = state::AccountProperties;
			template<typename TValueType>
			static auto GetResult(const TValueType& restrictions)
			{
				return restrictions.restriction(Equivalence());
			}
			static auto GetResult(const ValueType& restrictions){
				return restrictions.property(TKind::Property_Type);
			}

			static auto Equivalence(){
				if((uint8_t)TKind::Property_Type & (uint8_t)model::PropertyType::TransactionType)
					return static_cast<model::AccountRestrictionFlags>(TKind::Property_Type) | model::AccountRestrictionFlags::Outgoing;
				return static_cast<model::AccountRestrictionFlags>(TKind::Property_Type);
			}
		};
		template<typename TKind>
		struct KindDecoder<TKind, typename std::enable_if<!std::is_member_pointer<decltype(&TKind::Restriction_Flags)>::value>::type>{
			using type = model::AccountRestrictionFlags;
			using ValueType = state::AccountRestrictions;
			template<typename TValueType>
			static auto GetResult(const TValueType& restrictions)
			{
				return restrictions.property(Equivalence());
			}
			static auto GetResult(const ValueType& restrictions){
				return restrictions.restriction(TKind::Restriction_Flags);
			}
			static auto Equivalence(){
				if((uint8_t)TKind::Restriction_Flags & (uint8_t)model::AccountRestrictionFlags::TransactionType)
				{
					return (uint8_t)TKind::Restriction_Flags & (uint8_t)model::AccountRestrictionFlags::Block ?
																											  model::PropertyType::TransactionType | model::PropertyType::Block :
																											  model::PropertyType::TransactionType;
				}

				return static_cast<model::PropertyType>(TKind::Restriction_Flags);
			}
		};

		template<typename TOriginCacheType, typename TDestinationCacheType, typename TRestrictionValueTraits>
		void AssertCacheEntries(
				size_t expectedSize,
				const TOriginCacheType& originDelta,
				const TDestinationCacheType& destinationDelta,
				const std::vector<EntryType<TRestrictionValueTraits>>& entries) {
			// Assert:
			for(auto &entry : entries) {
				auto address = test::ConvertToAddress(entry.first);
				EXPECT_FALSE(originDelta.contains(address));
				auto iter = destinationDelta.find(address);
				EXPECT_TRUE(!!iter.tryGet());
				const auto& restrictions = iter.get();
				const auto& restriction = KindDecoder<TRestrictionValueTraits>::GetResult(restrictions);
				for(auto &val : entry.second)
					EXPECT_TRUE(restriction.contains(utils::ToVector(val)));
				EXPECT_EQ(expectedSize, restriction.values().size());
			}
		}



		template<typename TPropertyValueTraits>
		auto GenerateEntries(int numberOfRecords, int numberOfValuesPerRecord) {

			std::vector<EntryType<TPropertyValueTraits>> result;
			for(auto i = 0; i < numberOfRecords; i++) {
				result.push_back(std::make_pair(test::GenerateRandomByteArray<Key>(), test::GenerateUniqueRandomDataVector<typename TPropertyValueTraits::ValueType>(numberOfValuesPerRecord)));
			}
			return result;
		}

		template<typename TOperationTraitsProperty, typename TOperationTraitsAccountRestriction, typename TPropertyValueTraits, typename TAccountRestrictionValueTraits>
		void RunTest(
				size_t expectedSize,
				NotifyMode notifyMode,
				size_t numInitialValues,
				size_t numEntries) {
			// Arrange:
			// Prepare Configurations
			auto initialConfiguration = test::MutableBlockchainConfiguration();
			auto modifiedConfiguration = test::MutableBlockchainConfiguration();

			auto pluginConfig = config::AccountRestrictionConfiguration::Uninitialized();
			pluginConfig.Enabled = false;
			pluginConfig.MaxAccountRestrictionValues = 10;

			auto pluginConfigNew = config::AccountRestrictionConfiguration::Uninitialized();
			pluginConfigNew.Enabled = true;
			pluginConfigNew.MaxAccountRestrictionValues = 10;

			initialConfiguration.Network.SetPluginConfiguration(pluginConfig);
			modifiedConfiguration.Network.SetPluginConfiguration(pluginConfigNew);

			auto oldConfiguration = initialConfiguration.ToConst();
			modifiedConfiguration.PreviousConfiguration = &oldConfiguration;
			modifiedConfiguration.ActivationHeight = Height(777);

			ObserverTestContext context(notifyMode, Height(777), modifiedConfiguration.ToConst());


			auto entries = GenerateEntries<TPropertyValueTraits>(numEntries,numInitialValues);
			for(auto& entry : entries) {
				if (notifyMode == NotifyMode::Commit)
					test::PopulateCache<TPropertyValueTraits, TOperationTraitsProperty>(
							context.cache(), entry.first, entry.second);
				else
					test::PopulateAccountRestrictionCache<TAccountRestrictionValueTraits, TOperationTraitsAccountRestriction>(
							context.cache(), test::ConvertToAddress(entry.first), entry.second);
			}
			auto notification = test::CreateBlockNotification();
			auto pObserver = CreatePropertyMigrationObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);



			// Assert:
			if(notifyMode == NotifyMode::Commit)
				AssertCacheEntries<cache::PropertyCacheDelta, cache::AccountRestrictionCacheDelta, TPropertyValueTraits>(
					expectedSize,
					context.cache().sub<cache::PropertyCache>(),
					context.cache().sub<cache::AccountRestrictionCache>(),
					entries);
			else
				AssertCacheEntries<cache::AccountRestrictionCacheDelta, cache::PropertyCacheDelta, TPropertyValueTraits>(
						expectedSize,
						context.cache().sub<cache::AccountRestrictionCache>(),
						context.cache().sub<cache::PropertyCache>(),
						entries);
		}

		template<typename TOperationTraitsIn, typename TOperationTraitsOut, typename TTraitsIn, typename TTraitsOut>
		void RunTest(size_t expectedSize, NotifyMode notifyMode) {
			// Act:
			RunTest<TOperationTraitsIn, TOperationTraitsOut, TTraitsIn, TTraitsOut>(expectedSize, notifyMode, Num_Default_Entries, Num_Default_Entries);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TOperationTraitsProperty, typename TOperationTraitsAccountRestriction, typename TTraitsProperty, typename TTraitsAccountRestriction> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address_Allow) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyAllowTraits,test::AllowTraits,  test::BaseAddressPropertyTraits, test::BaseAccountAddressRestrictionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Mosaic_Allow) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyAllowTraits,test::AllowTraits,  test::BaseMosaicPropertyTraits, test::BaseAccountMosaicRestrictionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_TransactionType_Allow) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyAllowTraits,test::AllowTraits,  test::BaseTransactionTypePropertyTraits, test::BaseAccountOperationRestrictionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Address_Block) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyBlockTraits,test::BlockTraits,  test::BaseAddressPropertyTraits, test::BaseAccountAddressRestrictionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Mosaic_Block) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyBlockTraits,test::BlockTraits,  test::BaseMosaicPropertyTraits, test::BaseAccountMosaicRestrictionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_TransactionType_Block) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::PropertyBlockTraits,test::BlockTraits,  test::BaseTransactionTypePropertyTraits, test::BaseAccountOperationRestrictionTraits>(); } \
	template<typename TOperationTraitsProperty, typename TOperationTraitsAccountRestriction, typename TTraitsProperty, typename TTraitsAccountRestriction> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(CanVerifyMigrationOnPluginActivation)
	{
		RunTest<TOperationTraitsProperty, TOperationTraitsAccountRestriction, TTraitsProperty, TTraitsAccountRestriction>(Num_Default_Entries, NotifyMode::Commit);
	}
	// endregion
}}
