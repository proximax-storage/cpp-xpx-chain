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

#pragma once
#include "AccountRestrictionBaseSets.h"
#include "AccountRestrictionCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the account restriction cache view.
	using AccountRestrictionCacheViewMixins =


	struct LockFundCacheViewMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<AccountRestrictionCacheTypes::PrimaryTypes::BaseSetType, AccountRestrictionCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::AccountRestrictionConfiguration>;
	};
	/// Basic view on top of the account restriction cache.
	class BasicAccountRestrictionCacheView
			: public utils::MoveOnly
			, public AccountRestrictionCacheViewMixins::PrimaryMixins::Size
			, public AccountRestrictionCacheViewMixins::PrimaryMixins::Contains
			, public AccountRestrictionCacheViewMixins::PrimaryMixins::Iteration
			, public AccountRestrictionCacheViewMixins::PrimaryMixins::ConstAccessor
			, public AccountRestrictionCacheViewMixins::PrimaryMixins::PatriciaTreeView
			, public AccountRestrictionCacheViewMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = AccountRestrictionCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a restrictionSets and \a networkIdentifier.
		BasicAccountRestrictionCacheView(
				const AccountRestrictionCacheTypes::BaseSets& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: AccountRestrictionCacheViewMixins::PrimaryMixins::Size(restrictionSets.Primary)
				, AccountRestrictionCacheViewMixins::PrimaryMixins::Contains(restrictionSets.Primary)
				, AccountRestrictionCacheViewMixins::PrimaryMixins::Iteration(restrictionSets.Primary)
				, AccountRestrictionCacheViewMixins::PrimaryMixins::ConstAccessor(restrictionSets.Primary)
				, AccountRestrictionCacheViewMixins::PrimaryMixins::PatriciaTreeView(restrictionSets.PatriciaTree.get())
				, AccountRestrictionCacheViewMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// View on top of the account restriction cache.
	class AccountRestrictionCacheView : public ReadOnlyViewSupplier<BasicAccountRestrictionCacheView> {
	public:
		/// Creates a view around \a restrictionSets and \a networkIdentifier.
		AccountRestrictionCacheView(
				const AccountRestrictionCacheTypes::BaseSets& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(restrictionSets, pConfigHolder)
		{}
	};
}}
