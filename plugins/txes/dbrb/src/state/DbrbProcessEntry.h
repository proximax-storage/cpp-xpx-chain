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
		explicit DbrbProcessEntry(const dbrb::ProcessId& processId, Timestamp expirationTime, VersionType version = 1)
			: m_processId(processId)
			, m_expirationTime(std::move(expirationTime))
			, m_version(version)
		{}

	public:
		VersionType version() const {
			return m_version;
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

	private:
		VersionType m_version;
		dbrb::ProcessId m_processId;
		Timestamp m_expirationTime;
	};
}}
