/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/dbrb/DbrbUtils.h"

namespace catapult { namespace state {

	class DbrbProcessEntry {
	public:
		explicit DbrbProcessEntry(const dbrb::ProcessId& processId, Timestamp expirationTime, VersionType version = 1, const BlockDuration& banPeriod = BlockDuration(0))
			: m_processId(processId)
			, m_expirationTime(std::move(expirationTime))
			, m_version(version)
			, m_banPeriod(banPeriod)
		{}

	public:
		VersionType version() const {
			return m_version;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		const dbrb::ProcessId& processId() const {
			return m_processId;
		}

		void setExpirationTime(const Timestamp& expirationTime) {
			m_expirationTime = expirationTime;
		}

		const Timestamp& expirationTime() const {
			return m_expirationTime;
		}

		void setBanPeriod(const BlockDuration& banPeriod) {
			m_banPeriod = banPeriod;
		}

		void decrementBanPeriod() {
			if (m_banPeriod > BlockDuration(0))
				m_banPeriod = m_banPeriod - BlockDuration(1);
		}

		const BlockDuration& banPeriod() const {
			return m_banPeriod;
		}

	private:
		VersionType m_version;
		dbrb::ProcessId m_processId;
		Timestamp m_expirationTime;
		BlockDuration m_banPeriod;
	};
}}
