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
#include <src/config/AccountRestrictionConfiguration.h>
#include "AccountRestrictionBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace cache {

	/// Mixins used by the account restriction cache delta.
	struct AccountRestrictionCacheDeltaMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<AccountRestrictionCacheTypes::PrimaryTypes::BaseSetDeltaType, AccountRestrictionCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::AccountRestrictionConfiguration>;
	};

	/// Basic delta on top of the account restriction cache.
	class BasicAccountRestrictionCacheDelta
			: public utils::MoveOnly
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::Size
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::Contains
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::BasicInsertRemove
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::DeltaElements
			, public AccountRestrictionCacheDeltaMixins::PrimaryMixins::BroadIteration
			, public AccountRestrictionCacheDeltaMixins::ConfigBasedEnable{
	public:
		using ReadOnlyView = AccountRestrictionCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a restrictionSets and \a networkIdentifier.
		BasicAccountRestrictionCacheDelta(
				const AccountRestrictionCacheTypes::BaseSetDeltaPointers& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: AccountRestrictionCacheDeltaMixins::PrimaryMixins::Size(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::Contains(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta(*restrictionSets.pPrimary, restrictionSets.pPatriciaTree)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::BasicInsertRemove(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::DeltaElements(*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::PrimaryMixins::BroadIteration (*restrictionSets.pPrimary)
				, AccountRestrictionCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pAccountRestrictionEntries(restrictionSets.pPrimary)
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		using AccountRestrictionCacheDeltaMixins::PrimaryMixins::ConstAccessor::find;
		using AccountRestrictionCacheDeltaMixins::PrimaryMixins::MutableAccessor::find;

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		AccountRestrictionCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pAccountRestrictionEntries;
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// Delta on top of the account restriction cache.
	class AccountRestrictionCacheDelta : public ReadOnlyViewSupplier<BasicAccountRestrictionCacheDelta> {
	public:
		/// Creates a delta around \a restrictionSets and \a networkIdentifier.
		AccountRestrictionCacheDelta(
				const AccountRestrictionCacheTypes::BaseSetDeltaPointers& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(restrictionSets, pConfigHolder)
		{}
	};
}}
