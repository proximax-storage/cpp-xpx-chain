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
		void LogProcess(const char* prefix, const Key& process, const Timestamp& timestamp, const BlockDuration& banPeriod, std::ostringstream& out) {
			auto time = std::chrono::system_clock::to_time_t(utils::ToTimePoint(timestamp));
			char buffer[40];
			std::strftime(buffer, 40 ,"%F %T", std::localtime(&time));
			out << std::endl << prefix << process << " expires at: " << buffer;
			if (banPeriod > BlockDuration(0))
				out << " (banned for " << banPeriod << " blocks)";
		}
	}

	dbrb::ViewData DbrbViewFetcherImpl::getView(Timestamp timestamp) const {
		dbrb::ViewData view;
		for (auto iter = m_expirationTimes.rbegin(); iter != m_expirationTimes.rend(); ++iter) {
			if (iter->first <= timestamp)
				break;

			for (const auto& id : iter->second) {
				if (m_banPeriods.at(id) == BlockDuration(0))
					view.emplace(id);
			}
		}

    	return view;
    }

	Timestamp DbrbViewFetcherImpl::getExpirationTime(const dbrb::ProcessId& processId) const {
		auto iter = m_processes.find(processId);
		if (iter != m_processes.end())
			return iter->second;

		return Timestamp(0);
    }

	BlockDuration DbrbViewFetcherImpl::getBanPeriod(const dbrb::ProcessId& processId) const {
		auto iter = m_banPeriods.find(processId);
		if (iter != m_banPeriods.end())
			return iter->second;

		return BlockDuration(0);
    }

	void DbrbViewFetcherImpl::addDbrbProcess(const state::DbrbProcessEntry& entry) {
		m_processes[entry.processId()] = entry.expirationTime();
		m_expirationTimes[entry.expirationTime()].emplace(entry.processId());
		m_banPeriods[entry.processId()] = entry.banPeriod();
    }

	void DbrbViewFetcherImpl::clear() {
		m_processes.clear();
		m_expirationTimes.clear();
		m_banPeriods.clear();
    }

	void DbrbViewFetcherImpl::logAllProcesses() const {
		std::ostringstream out;
		out << std::endl << "DBRB processes (" << m_processes.size() << "):";
		for (const auto& [id, expirationTime] : m_processes)
			LogProcess("DBRB process: ", id, expirationTime, m_banPeriods.at(id), out);
		CATAPULT_LOG(debug) << out.str();
    }

	void DbrbViewFetcherImpl::logView(const dbrb::ViewData& view) const {
		std::ostringstream out;
		out << std::endl << "current DBRB view (" << view.size() << "):";
		for (const auto& processId : view) {
			auto iter = m_processes.find(processId);
			if (iter != m_processes.cend()) {
				LogProcess("DBRB view member: ", processId, iter->second, m_banPeriods.at(processId), out);
			} else {
				out << std::endl << processId << " is not a member of the view";
			}
		}
		CATAPULT_LOG(debug) << out.str();
    }
}}
