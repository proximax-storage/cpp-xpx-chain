/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionBuilderNetworkConfigCapability.h"
#include "tests/test/local/RealTransactionFactory.h"

namespace catapult { namespace test {

	// region add / create


	model::UniqueEntityPtr<model::Transaction> TransactionBuilderNetworkConfigCapability::createNetworkConfig(const NetworkConfigDescriptor& descriptor, Timestamp deadline)
	{
		auto pTransaction = CreateNetworkConfigTransaction(accounts().getNemesisKeyPair(), descriptor.NetworkConfig, descriptor.SupportedEntities, descriptor.BlocksBeforeActive);
		return SignWithDeadline(std::move(pTransaction), accounts().getNemesisKeyPair(), deadline);
	}

	void TransactionBuilderNetworkConfigCapability::registerHooks() {
		m_builder.registerDescriptor(DescriptorTypes::Network_Config, [self = ptr<TransactionBuilderNetworkConfigCapability>()](auto& pDescriptor, auto deadline){
		  return self->createNetworkConfig(CastToDescriptor<NetworkConfigDescriptor>(pDescriptor), deadline);
		});
	}
	void TransactionBuilderNetworkConfigCapability::addNetworkConfigUpdate(std::string networkConfig, std::string supportedEntities, BlockDuration blocksBeforeActive)
	{
		auto descriptor = NetworkConfigDescriptor{networkConfig, supportedEntities, blocksBeforeActive};
		add(DescriptorTypes::Network_Config, std::move(descriptor));
	}

	// endregion
}}
