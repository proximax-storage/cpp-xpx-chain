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

#include "AccountStateCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void AccountStateCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("AccountStateCacheSummaryCacheStorage does not support saveAll");
	}

	void AccountStateCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		const auto& delta = cacheDelta.sub<AccountStateCache>();
		const auto& highValueAddresses = delta.highValueAddresses();
		io::Write64(output, highValueAddresses.size());
		for (const auto& address : highValueAddresses)
			output.write(address);

		const auto& addressesToUpdate = delta.updatedAddresses();
		io::Write64(output, addressesToUpdate.size());
		for (const auto& address : addressesToUpdate)
			output.write(address);

		output.flush();
	}

	void AccountStateCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		auto numAddresses = io::Read64(input);

		model::AddressSet highValueAddresses;
		for (auto i = 0u; i < numAddresses; ++i) {
			Address address;
			input.read(address);
			highValueAddresses.insert(address);
		}

		numAddresses = io::Read64(input);

		model::AddressSet addressesToUpdate;
		for (auto i = 0u; i < numAddresses; ++i) {
			Address address;
			input.read(address);
			addressesToUpdate.insert(address);
		}

		cache().init(std::move(highValueAddresses), std::move(addressesToUpdate));
	}

	AccountStateCacheSubCachePlugin::AccountStateCacheSubCachePlugin(
			const CacheConfiguration& config,
			const AccountStateCacheTypes::Options& options)
			: BaseAccountStateCacheSubCachePlugin(std::make_unique<AccountStateCache>(config, options))
	{}
}}
