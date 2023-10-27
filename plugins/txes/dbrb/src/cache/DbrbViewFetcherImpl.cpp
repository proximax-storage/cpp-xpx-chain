/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbViewFetcherImpl.h"
#include "src/cache/DbrbViewCache.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace cache {

	namespace {
		void LogProcess(const char* prefix, const Key& process, const Timestamp& timestamp, std::ostringstream& out) {
			auto time = std::chrono::system_clock::to_time_t(utils::ToTimePoint(timestamp));
			char buffer[40];
			std::strftime(buffer, 40 ,"%F %T", std::localtime(&time));
			out << std::endl << prefix << process << " expires at: " << buffer;
		}

		void LogAllProcesses(const std::map<dbrb::ProcessId, Timestamp>& processes) {
			std::ostringstream out;
			out << std::endl << "DBRB processes (" << processes.size() << "):";
			for (const auto& pair : processes)
				LogProcess("DBRB process: ", pair.first, pair.second, out);
			CATAPULT_LOG(trace) << out.str();
		}

		void LogCurrentView(const dbrb::ViewData& view, const std::map<dbrb::ProcessId, Timestamp>& processes) {
			std::ostringstream out;
			out << std::endl << "current DBRB view (" << view.size() << "):";
			for (const auto& processId : view)
				LogProcess("DBRB view member: ", processId, processes.at(processId), out);
			CATAPULT_LOG(trace) << out.str();
		}
	}

	dbrb::ViewData DbrbViewFetcherImpl::getView(Timestamp timestamp) const {
		dbrb::ViewData view;
		for (auto iter = m_expirationTimes.rbegin(); iter != m_expirationTimes.rend(); ++iter) {
			if (iter->first <= timestamp)
				break;

			view.insert(iter->second.begin(), iter->second.end());
		}

		LogAllProcesses(m_processes);
		LogCurrentView(view, m_processes);

    	return view;
    }

	Timestamp DbrbViewFetcherImpl::getExpirationTime(const dbrb::ProcessId& processId) const {
		auto iter = m_processes.find(processId);
		if (iter != m_processes.end())
			return iter->second;

		return Timestamp();
    }

	void DbrbViewFetcherImpl::addDbrbProcess(const state::DbrbProcessEntry& entry) {
		m_processes[entry.processId()] = entry.expirationTime();
		m_expirationTimes[entry.expirationTime()].emplace(entry.processId());
    }

	void DbrbViewFetcherImpl::clear() {
		m_processes.clear();
		m_expirationTimes.clear();
    }
}}
