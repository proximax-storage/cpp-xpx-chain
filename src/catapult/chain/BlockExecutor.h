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
#include "catapult/model/Elements.h"
#include "catapult/observers/ObserverTypes.h"
#include <memory>

namespace catapult { namespace model { struct Block; } }

namespace catapult { namespace chain {
	/// Block execution context.
	struct BlockExecutionContext {
	public:
		/// Creates a block execution context around \a observer, \a resolvers, \a configHolder and \a state.
		BlockExecutionContext(
				const observers::EntityObserver& observer,
				const model::ResolverContext& resolvers,
				const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder,
				observers::ObserverState& state)
				: Observer(observer)
				, Resolvers(resolvers)
				, ConfigHolder(configHolder)
				, State(state)
		{}

	public:
		/// Observer to execute the block.
		const observers::EntityObserver& Observer;

		/// Alias resolvers.
		const model::ResolverContext& Resolvers;

		/// Config holder.
		std::shared_ptr<config::BlockchainConfigurationHolder> ConfigHolder;

		/// State to update during observation.
		observers::ObserverState& State;
	};

	/// Executes \a blockElement using the specified execution context (\a executionContext).
	void ExecuteBlock(const model::BlockElement& blockElement, const BlockExecutionContext& executionContext);

	/// Rollbacks \a blockElement using the specified execution context (\a executionContext).
	void RollbackBlock(const model::BlockElement& blockElement, const BlockExecutionContext& executionContext);
}}
