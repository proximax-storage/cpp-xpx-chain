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
#include "HandlerTypes.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/model/TransactionPlugin.h"
#include "catapult/utils/Logging.h"
#include <functional>

namespace catapult { namespace handlers {

	/// Creates a push handler that forwards a received entity range to \a rangeHandler
	/// given a \a registry composed of supported transaction types.
	template<typename TEntity>
	auto CreatePushEntityHandler(const model::TransactionRegistry& registry, const RangeHandler<TEntity>& rangeHandler) {
		return [rangeHandler, &registry](const ionet::Packet& packet, const auto& context) {
			auto range = ionet::ExtractEntitiesFromPacket<TEntity>(packet, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			});
			if (range.empty()) {
				CATAPULT_LOG(warning) << "rejecting empty range: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;
			rangeHandler({ std::move(range), context.key() });
		};
	}

	/// Creates a push handler that forwards a received entity range to \a rangeHandler in batches
	/// of size \a batchSize, given a \a registry composed of supported transaction types.
	template<typename TEntity>
	auto CreateBatchedPushEntityHandler(const model::TransactionRegistry& registry, size_t batchSize, const RangeHandler<TEntity>& rangeHandler) {
		return [rangeHandler, &registry, batchSize](const ionet::Packet& packet, const auto& context) {
			auto ranges = ionet::ExtractEntityBatchesFromPacket<TEntity>(packet, batchSize, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			});
			if (ranges.empty()) {
				CATAPULT_LOG(warning) << "rejecting empty range: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;
			for (auto i = 0u; i < ranges.size(); ++i)
				rangeHandler({ std::move(ranges[i]), context.key() });
		};
	}
}}
