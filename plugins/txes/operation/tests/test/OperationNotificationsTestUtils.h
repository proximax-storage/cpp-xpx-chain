/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationTestUtils.h"
#include "src/model/OperationNotifications.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Start operation notification builder.
	struct StartOperationNotificationBuilder {
	public:
		/// Creates secret lock notification builder.
		StartOperationNotificationBuilder() {
			m_operationToken = GenerateRandomByteArray<Hash256>();
			m_initiator = GenerateRandomByteArray<Key>();
			m_duration = GenerateRandomValue<BlockDuration>();
			for (auto i = 0u; i < 3u; ++i)
				m_mosaics.push_back({GenerateRandomValue<UnresolvedMosaicId>(), GenerateRandomValue<Amount>()});
			for (auto i = 0u; i < 4u; ++i)
				m_executors.push_back(GenerateRandomByteArray<Key>());
		}

		/// Creates a notification.
		auto notification() {
			return model::StartOperationNotification<1>(
				m_operationToken,
				m_initiator,
				m_executors.data(),
				m_executors.size(),
				m_mosaics.data(),
				m_mosaics.size(),
				m_duration);
		}

		/// Prepares the builder using \a entry.
		void prepare(const state::OperationEntry& entry) {
			m_operationToken = entry.OperationToken;
			m_initiator = entry.Account;

			m_mosaics.clear();
			m_mosaics.reserve(entry.Mosaics.size());
			for (const auto& pair : entry.Mosaics)
				m_mosaics.push_back({test::UnresolveXor(pair.first), pair.second});

			m_executors.clear();
			m_executors.reserve(entry.Executors.size());
			for (const auto& executor : entry.Executors)
				m_executors.push_back(executor);
		}

	private:
		Hash256 m_operationToken;
		Key m_initiator;
		std::vector<model::UnresolvedMosaic> m_mosaics;
		std::vector<Key> m_executors;
		BlockDuration m_duration;
	};
}}
