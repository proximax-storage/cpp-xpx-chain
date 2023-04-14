/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbViewFetcherImpl.h"
#include "src/cache/DbrbViewCache.h"

namespace catapult { namespace cache {

	dbrb::ViewData DbrbViewFetcherImpl::getView(Timestamp timestamp) const {
		dbrb::ViewData view;
		for (auto iter = m_expirationTimes.rbegin(); iter != m_expirationTimes.rend(); ++iter) {
			if (iter->first <= timestamp)
				break;

			view.insert(iter->second.begin(), iter->second.end());
		}

    	return view;
    }

	dbrb::ViewData DbrbViewFetcherImpl::getExpiredDbrbProcesses(Timestamp timestamp) {
		dbrb::ViewData expiredProcesses;
		for (auto iter = m_expirationTimes.begin(); iter != m_expirationTimes.end(); ++iter) {
			if (iter->first > timestamp)
				break;

			expiredProcesses.insert(iter->second.begin(), iter->second.end());
		}

		return expiredProcesses;
	}

	Timestamp DbrbViewFetcherImpl::getExpirationTime(const dbrb::ProcessId& processId) const {
		auto iter = m_processes.find(processId);
		if (iter != m_processes.end())
			return iter->second;

		return Timestamp();
    }

	void DbrbViewFetcherImpl::addOrUpdateDbrbProcess(const state::DbrbProcessEntry& entry) {
		removeExpirationTime(entry.processId());
		m_processes[entry.processId()] = entry.expirationTime();
		m_expirationTimes[entry.expirationTime()].emplace(entry.processId());
    }

	void DbrbViewFetcherImpl::removeDbrbProcess(const dbrb::ProcessId& processId) {
		removeExpirationTime(processId);
		m_processes.erase(processId);
    }

	void DbrbViewFetcherImpl::removeExpirationTime(const dbrb::ProcessId& processId) {
		auto processIter = m_processes.find(processId);
		if (processIter != m_processes.end()) {
			auto expirationTimeIter = m_expirationTimes.find(processIter->second);
			expirationTimeIter->second.erase(processId);
			if (expirationTimeIter->second.empty())
				m_expirationTimes.erase(expirationTimeIter);
		}
    }
}}
