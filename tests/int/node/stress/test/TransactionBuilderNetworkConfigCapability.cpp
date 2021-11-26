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
