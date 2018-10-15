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

namespace catapult { namespace cache {

	using AccountStateBasicCache = BasicCache<
		AccountStateCacheDescriptor,
		AccountStateCacheTypes::BaseSets,
		AccountStateCacheTypes::Options,
		const model::AddressSet&>;

	/// Cache composed of stateful account information.
	class BasicAccountStateCache : public AccountStateBasicCache {
	public:
		/// Creates a cache around \a config and \a options.
		explicit BasicAccountStateCache(const CacheConfiguration& config, const AccountStateCacheTypes::Options& options)
				: BasicAccountStateCache(config, options, std::make_unique<model::AddressSet>())
		{}

	private:
		BasicAccountStateCache(
				const CacheConfiguration& config,
				const AccountStateCacheTypes::Options& options,
				std::unique_ptr<model::AddressSet>&& pHighValueAddresses)
				: AccountStateBasicCache(config, AccountStateCacheTypes::Options(options), *pHighValueAddresses)
				, m_pHighValueAddresses(std::move(pHighValueAddresses))
		{}

	public:
		/// Commits all pending changes to the underlying storage.
		/// \note This hides AccountStateBasicCache::commit.
		void commit(const CacheDeltaType& delta) {
			// high value addresses need to be captured before committing because committing clears the deltas
			auto highValueAddresses = delta.highValueAddresses();
			AccountStateBasicCache::commit(delta);
			*m_pHighValueAddresses = std::move(highValueAddresses);
		}

	private:
		// unique pointer to allow set reference to be valid after moves of this cache
		std::unique_ptr<model::AddressSet> m_pHighValueAddresses;
	};

	/// Synchronized cache composed of stateful account information.
	class AccountStateCache : public SynchronizedCache<BasicAccountStateCache> {
	public:
		DEFINE_CACHE_CONSTANTS(AccountState)

	public:
		/// Creates a cache around \a config and \a options.
		AccountStateCache(const CacheConfiguration& config, const AccountStateCacheTypes::Options& options)
				: SynchronizedCache<BasicAccountStateCache>(BasicAccountStateCache(config, options))
				, m_networkIdentifier(options.NetworkIdentifier)

		{}

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		model::NetworkIdentifier m_networkIdentifier;
	};
}}
