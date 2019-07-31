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
#include "AccountStateCacheDelta.h"
#include "AccountStateCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/LocalNodeConfigurationHolder.h"

namespace catapult { namespace cache {

	using AccountStateBasicCache = BasicCache<
		AccountStateCacheDescriptor,
		AccountStateCacheTypes::BaseSets,
		AccountStateCacheTypes::Options,
		const model::AddressSet&,
		model::AddressSet&>;

	/// Cache composed of stateful account information.
	class BasicAccountStateCache : public AccountStateBasicCache {
	public:
		/// Creates a cache around \a config and \a options.
		explicit BasicAccountStateCache(const CacheConfiguration& config, const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder)
				: BasicAccountStateCache(config, pConfigHolder, std::make_unique<model::AddressSet>(), std::make_unique<model::AddressSet>())
		{}

	private:
		BasicAccountStateCache(
				const CacheConfiguration& config,
				const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder,
				std::unique_ptr<model::AddressSet>&& pHighValueAddresses,
				std::unique_ptr<model::AddressSet>&& pAddressesToUpdate)
				: AccountStateBasicCache(config, AccountStateCacheTypes::Options{ pConfigHolder }, *pHighValueAddresses, *pAddressesToUpdate)
				, m_pHighValueAddresses(std::move(pHighValueAddresses))
				, m_pAddressesToUpdate(std::move(pAddressesToUpdate))
		{}

	public:
		/// Initializes the cache with \a highValueAddresses and \a addressesToUpdate.
		void init(model::AddressSet&& highValueAddresses, model::AddressSet&& addressesToUpdate) {
			*m_pHighValueAddresses = std::move(highValueAddresses);
			*m_pAddressesToUpdate = std::move(addressesToUpdate);
		}

		/// Commits all pending changes to the underlying storage.
		/// \note This hides AccountStateBasicCache::commit.
		void commit(const CacheDeltaType& delta) {
			// high value addresses need to be captured before committing because committing clears the deltas
			auto highValueAddresses = delta.highValueAddresses();
			delta.commitSnapshots();
			delta.addUpdatedAddresses(*m_pAddressesToUpdate);
			AccountStateBasicCache::commit(delta);
			*m_pHighValueAddresses = std::move(highValueAddresses);
		}

	private:
		// unique pointer to allow set reference to be valid after moves of this cache
		std::unique_ptr<model::AddressSet> m_pHighValueAddresses;
		std::unique_ptr<model::AddressSet> m_pAddressesToUpdate;
	};

	/// Synchronized cache composed of stateful account information.
	class AccountStateCache : public SynchronizedCacheWithInit<BasicAccountStateCache> {
	public:
		DEFINE_CACHE_CONSTANTS(AccountState)

	public:
		/// Creates a cache around \a config and \a options.
		AccountStateCache(const CacheConfiguration& config, const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder)
				: SynchronizedCacheWithInit<BasicAccountStateCache>(BasicAccountStateCache(config, pConfigHolder))
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_pConfigHolder->Config().BlockChain.Network.Identifier;
		}

		/// Gets the network importance grouping.
		uint64_t importanceGrouping() const {
			return m_pConfigHolder->Config().BlockChain.ImportanceGrouping;
		}

	private:
		std::shared_ptr<config::LocalNodeConfigurationHolder> m_pConfigHolder;
	};
}}
