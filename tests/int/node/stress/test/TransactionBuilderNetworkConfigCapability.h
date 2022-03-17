/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionsBuilder.h"
#include "TransactionBuilderCapability.h"
#include "TransactionsGenerator.h"


namespace catapult { namespace test {


	class TransactionBuilderNetworkConfigCapability : public TransactionBuilderCapability
	{

	private:
		struct NetworkConfigDescriptor {
			std::string NetworkConfig;
			std::string SupportedEntities;
			BlockDuration BlocksBeforeActive;
		};

	public:
		/// Adds a network config update.
		void addNetworkConfigUpdate(std::string networkConfig, std::string supportedEntities, BlockDuration blocksBeforeActive);

	public:
		TransactionBuilderNetworkConfigCapability(TransactionsBuilder& builder) : TransactionBuilderCapability(builder)
		{

		}
		void registerHooks() override;
	private:
		model::UniqueEntityPtr<model::Transaction> createNetworkConfig(const NetworkConfigDescriptor& descriptor, Timestamp deadline);
	};
}}
