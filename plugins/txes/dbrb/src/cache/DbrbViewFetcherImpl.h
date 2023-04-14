/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/CatapultCache.h"
#include "catapult/dbrb/DbrbViewFetcher.h"
#include "catapult/utils/Hashers.h"
#include <map>

namespace catapult { namespace state { class DbrbProcessEntry; }}

namespace catapult { namespace cache {

    class DbrbViewFetcherImpl : public dbrb::DbrbViewFetcher {
    public:
        explicit DbrbViewFetcherImpl() = default;

    public:
		dbrb::ViewData getView(Timestamp timestamp) const override;
		Timestamp getExpirationTime(const dbrb::ProcessId& processId) const override;

	public:
		void addOrUpdateDbrbProcess(const state::DbrbProcessEntry& entry);
		void removeDbrbProcess(const dbrb::ProcessId& processId);
		dbrb::ViewData getExpiredDbrbProcesses(Timestamp timestamp);

	private:
		void removeExpirationTime(const dbrb::ProcessId& processId);

	private:
		std::map<dbrb::ProcessId, Timestamp> m_processes;
		std::map<Timestamp, std::unordered_set<dbrb::ProcessId, utils::ArrayHasher<dbrb::ProcessId>>> m_expirationTimes;
    };
}}
