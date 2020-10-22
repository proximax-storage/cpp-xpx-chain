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
#include "catapult/disruptor/DisruptorTypes.h"
#include "catapult/ionet/NodeInteractionResult.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include <set>

namespace catapult {
	namespace disruptor {
		struct ConsumerCompletionResult;
		class ConsumerInput;
	}
}

namespace catapult { namespace consumers {

	/// Converts a consumer completion \a result to a node interaction result with public key \a sourcePublicKey.
	ionet::NodeInteractionResult ToNodeInteractionResult(const Key& sourcePublicKey, const disruptor::ConsumerCompletionResult& result);

	model::NetworkConfiguration ParseConfig(const uint8_t* pConfig, uint16_t configSize);

	template<typename TTransaction>
	void AddConfig(model::NetworkConfigurations& configs, const Height& blockHeight, const TTransaction& transaction) {
		configs.emplace(
			blockHeight +  Height(transaction.ApplyHeightDelta.unwrap()),
			ParseConfig(transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize));
	}

	template<typename TTransaction>
	void AddConfig(std::set<Height>& configHeights, const Height& blockHeight, const TTransaction& transaction) {
		configHeights.insert(blockHeight +  Height(transaction.ApplyHeightDelta.unwrap()));
	}

	template<typename TContainer>
	bool ExtractConfigs(const disruptor::BlockElements& elements, TContainer& configs) {
		try {
			for (const auto& blockElement : elements) {
				for (const auto& transactionElement : blockElement.Transactions) {
					const auto& transaction = transactionElement.Transaction;
					auto type = transaction.Type;
					if (model::Entity_Type_Network_Config == type) {
						AddConfig(configs, blockElement.Block.Height,
								  static_cast<const model::NetworkConfigTransaction&>(transaction));
					} else if (model::Entity_Type_Aggregate_Complete == type || model::Entity_Type_Aggregate_Bonded == type) {
						const auto& aggregate = static_cast<const model::AggregateTransaction&>(transaction);
						for (const auto& subTransaction : aggregate.Transactions()) {
							if (model::Entity_Type_Network_Config == subTransaction.Type) {
								AddConfig(configs, blockElement.Block.Height,
										  static_cast<const model::EmbeddedNetworkConfigTransaction&>(subTransaction));
							}
						}
					}
				}
			}
		} catch (...) {
			return false;
		}

		return true;
	}
}}
