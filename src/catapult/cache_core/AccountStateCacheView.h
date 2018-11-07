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

#pragma once
#include "AccountStateCacheTypes.h"
#include "ReadOnlyAccountStateCache.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/model/ContainerTypes.h"

namespace catapult { namespace cache {

	/// Mixins used by the account state cache view.
	struct AccountStateCacheViewMixins {
	public:
		using KeyLookupAdapter = AccountStateCacheTypes::ComposedLookupAdapter<AccountStateCacheTypes::ComposableBaseSets>;

	private:
		using AddressMixins = BasicCacheMixins<AccountStateCacheTypes::PrimaryTypes::BaseSetType, AccountStateCacheDescriptor>;
		using KeyMixins = BasicCacheMixins<KeyLookupAdapter, KeyLookupAdapter>;

	public:
		using Size = AddressMixins::Size;
		using ContainsAddress = AddressMixins::Contains;
		using ContainsKey = ContainsMixin<
			AccountStateCacheTypes::KeyLookupMapTypes::BaseSetType,
			AccountStateCacheTypes::KeyLookupMapTypesDescriptor>;
		using Iteration = AddressMixins::Iteration;
		using ConstAccessorAddress = AddressMixins::ConstAccessorWithAdapter<AccountStateCacheTypes::ConstValueAdapter>;
		using ConstAccessorKey = KeyMixins::ConstAccessorWithAdapter<AccountStateCacheTypes::ConstValueAdapter>;
	};

	/// Basic view on top of the account state cache.
	class BasicAccountStateCacheView
			: public utils::MoveOnly
			, public AccountStateCacheViewMixins::Size
			, public AccountStateCacheViewMixins::ContainsAddress
			, public AccountStateCacheViewMixins::ContainsKey
			, public AccountStateCacheViewMixins::Iteration
			, public AccountStateCacheViewMixins::ConstAccessorAddress
			, public AccountStateCacheViewMixins::ConstAccessorKey {
	public:
		using ReadOnlyView = ReadOnlyAccountStateCache;

	public:
		/// Creates a view around \a accountStateSets, \a options, \a highValueAddresses and \a updatedAddresses.
		BasicAccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				model::AddressSet& updatedAddresses);

	private:
		BasicAccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				model::AddressSet& updatedAddresses,
				std::unique_ptr<AccountStateCacheViewMixins::KeyLookupAdapter>&& pKeyLookupAdapter);

	public:
		using AccountStateCacheViewMixins::ContainsAddress::contains;
		using AccountStateCacheViewMixins::ContainsKey::contains;

		using AccountStateCacheViewMixins::ConstAccessorAddress::get;
		using AccountStateCacheViewMixins::ConstAccessorKey::get;

		using AccountStateCacheViewMixins::ConstAccessorAddress::tryGet;
		using AccountStateCacheViewMixins::ConstAccessorKey::tryGet;

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const;

		/// Gets the network importance grouping.
		uint64_t importanceGrouping() const;

	public:
		/// Gets the number of high value addresses.
		size_t highValueAddressesSize() const;

	private:
		const model::NetworkIdentifier m_networkIdentifier;
		const uint64_t m_importanceGrouping;
		const model::AddressSet& m_highValueAddresses;
		model::AddressSet& m_updatedAddresses;
		std::unique_ptr<AccountStateCacheViewMixins::KeyLookupAdapter> m_pKeyLookupAdapter;
	};

	/// View on top of the account state cache.
	class AccountStateCacheView : public ReadOnlyViewSupplier<BasicAccountStateCacheView> {
	public:
		/// Creates a view around \a accountStateSets, \a options, \a highValueAddresses and \a updatedAddresses.
		AccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				model::AddressSet& updatedAddresses)
				: ReadOnlyViewSupplier(accountStateSets, options, highValueAddresses, updatedAddresses)
		{}
	};
}}
