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
#include "AccountStateBaseSets.h"
#include "AccountStateCacheSerializers.h"
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
		using AddressMixins = PatriciaTreeCacheMixins<AccountStateCacheTypes::PrimaryTypes::BaseSetType, AccountStateCacheDescriptor>;
		using KeyMixins = BasicCacheMixins<KeyLookupAdapter, KeyLookupAdapter>;

	public:
		using Size = AddressMixins::Size;
		using ContainsAddress = AddressMixins::Contains;
		using ContainsKey = ContainsMixin<
			AccountStateCacheTypes::KeyLookupMapTypes::BaseSetType,
			AccountStateCacheTypes::KeyLookupMapTypesDescriptor>;
		using Iteration = AddressMixins::Iteration;
		using ConstAccessorAddress = AddressMixins::ConstAccessor;
		using ConstAccessorKey = KeyMixins::ConstAccessor;
		using PatriciaTreeView = AddressMixins::PatriciaTreeView;
		using Enable = AddressMixins::Enable;
		using Height = KeyMixins::Height;
	};

	/// Basic view on top of the account state cache.
	class BasicAccountStateCacheView
			: public utils::MoveOnly
			, public AccountStateCacheViewMixins::Size
			, public AccountStateCacheViewMixins::ContainsAddress
			, public AccountStateCacheViewMixins::ContainsKey
			, public AccountStateCacheViewMixins::Iteration
			, public AccountStateCacheViewMixins::ConstAccessorAddress
			, public AccountStateCacheViewMixins::ConstAccessorKey
			, public AccountStateCacheViewMixins::PatriciaTreeView
			, public AccountStateCacheViewMixins::Enable
			, public AccountStateCacheViewMixins::Height {
	public:
		using ReadOnlyView = ReadOnlyAccountStateCache;

	public:
		/// Creates a view around \a accountStateSets, \a options, \a highValueAddresses and \a addressesToUpdate.
		BasicAccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				const model::AddressSet& addressesToUpdate);

	private:
		BasicAccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				const model::AddressSet& addressesToUpdate,
				std::unique_ptr<AccountStateCacheViewMixins::KeyLookupAdapter>&& pKeyLookupAdapter);

	public:
		using AccountStateCacheViewMixins::ContainsAddress::contains;
		using AccountStateCacheViewMixins::ContainsKey::contains;

		using AccountStateCacheViewMixins::ConstAccessorAddress::find;
		using AccountStateCacheViewMixins::ConstAccessorKey::find;

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const;

		/// Gets the network importance grouping.
		uint64_t importanceGrouping() const;

		/// Gets the harvesting mosaic id.
		MosaicId harvestingMosaicId() const;

	public:
		/// Gets all high value addresses.
		const model::AddressSet& highValueAddresses() const;

		/// Gets all addresses that we need to clean up.
		const model::AddressSet& addressesToUpdate() const;

	private:
		const model::NetworkConfiguration& networkConfig() const;

	private:
		const AccountStateCacheTypes::Options& m_options;
		const model::AddressSet& m_highValueAddresses;
		const model::AddressSet& m_addressesToUpdate;
		std::unique_ptr<AccountStateCacheViewMixins::KeyLookupAdapter> m_pKeyLookupAdapter;
	};

	/// View on top of the account state cache.
	class AccountStateCacheView : public ReadOnlyViewSupplier<BasicAccountStateCacheView> {
	public:
		/// Creates a view around \a accountStateSets, \a options, \a highValueAddresses and \a addressesToUpdate.
		AccountStateCacheView(
				const AccountStateCacheTypes::BaseSets& accountStateSets,
				const AccountStateCacheTypes::Options& options,
				const model::AddressSet& highValueAddresses,
				const model::AddressSet& addressesToUpdate)
				: ReadOnlyViewSupplier(accountStateSets, options, highValueAddresses, addressesToUpdate)
		{}
	};
}}
