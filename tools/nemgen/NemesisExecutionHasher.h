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
#include "catapult/types.h"
#include "NemesisConfiguration.h"
#include "NemesisTransactions.h"
#include "catapult/chain/BlockExecutor.h"
#include <string>
#include <memory>

namespace catapult {
	namespace config { class BlockchainConfigurationHolder; }
	namespace model { struct BlockElement; }
}

namespace catapult { namespace tools { namespace nemgen {

	/// Possible cache database cleanup modes.
	enum class CacheDatabaseCleanupMode {
		/// Perform no cleanup.
		None,

		/// Purge after execution.
		Purge
	};

	/// Nemesis block execution dependent hashes information.
	struct NemesisExecutionHashesDescriptor {
		/// Receipts hash.
		Hash256 ReceiptsHash;

		/// State hash.
		Hash256 StateHash;

		/// Textual summary including sub cache hashes.
		std::string Summary;
	};


	/// Calculates and logs the nemesis block execution dependent hashes after executing nemesis \a blockElement
	/// for network configured with \a config with specified cache database cleanup mode (\a databaseCleanupMode).
	NemesisExecutionHashesDescriptor CalculateAndLogNemesisExecutionHashes(
			const NemesisConfiguration& nemesisConfig,
			const model::BlockElement& blockElement,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			CacheDatabaseCleanupMode databaseCleanupMode,
			NemesisTransactions* transactions);

	namespace detail {
		observers::ObserverContext CreateObserverContext(
				const chain::BlockExecutionContext& executionContext,
				Height height,
				const Timestamp& timestamp,
				observers::NotifyMode mode);
		template<typename TWeakEntityInfoProvider, typename TWeakEntityInfoProcessor>
		void ObserveAll(
				const observers::EntityObserver& observer,
				observers::ObserverContext& context,
				TWeakEntityInfoProvider& entityInfos,
				TWeakEntityInfoProcessor entityPostOp) {
			for (const auto& entityInfo : entityInfos){
				auto result = entityPostOp(entityInfo);
				observer.notify(result, context);
			}
		}
	}

	class BaseEntityProvider {
	public:
		using OriginalIterator = NemesisTransactions::NemesisTransactionsView::Iterator;

		// Constructor
		BaseEntityProvider(NemesisTransactions::NemesisTransactionsView& container, const model::BlockElement& block)
			: m_container(container), m_extraElement(block) {}

		// Custom Iterator
		class Iterator {
		public:
			Iterator(OriginalIterator current, OriginalIterator end, const model::BlockElement* extraElement, bool atExtra)
				: m_current(current), m_end(end), m_extraElement(extraElement), m_atExtra(atExtra) {}

			// Dereference operator
			auto operator*() const {
				return m_atExtra ? model::WeakEntityInfo(m_extraElement->Block, m_extraElement->EntityHash, m_extraElement->Block) : model::WeakEntityInfo(*(*m_current).transaction, (*m_current).entityHash, m_extraElement->Block);
			}

			// Increment operator
			Iterator& operator++() {
				if (m_current != m_end) {
					++m_current;
					if (m_current == m_end) {
						m_atExtra = true; // Switch to returning extra element
					}
				} else {
					m_atExtra = false; // End iteration
				}
				return *this;
			}

			// Equality and inequality operators
			bool operator==(const Iterator& other) const {
				return m_current == other.m_current && m_atExtra == other.m_atExtra;
			}

			bool operator!=(const Iterator& other) const {
				return !(*this == other);
			}

		private:
			OriginalIterator m_current;
			OriginalIterator m_end;
			const model::BlockElement* m_extraElement;
			bool m_atExtra;
		};

		// Begin iterator
		Iterator begin() const {
			return Iterator(m_container.begin(), m_container.end(), &m_extraElement, false);
		}

		// End iterator
		Iterator end() const {
			return Iterator(m_container.end(), m_container.end(), &m_extraElement, false);
		}

	private:
		NemesisTransactions::NemesisTransactionsView& m_container;
		model::BlockElement m_extraElement;
	};

	template<typename TWeakEntityInfoProvider, typename TWeakEntityInfoProcessor>
	void ExecuteBlock(const model::BlockElement& blockElement, const chain::BlockExecutionContext& executionContext, TWeakEntityInfoProvider& entityInfoProvider, TWeakEntityInfoProcessor entityPostOp) {
		executionContext.State.Cache.setHeight(blockElement.Block.Height);
		auto context = detail::CreateObserverContext(executionContext, blockElement.Block.Height, blockElement.Block.Timestamp, observers::NotifyMode::Commit);
		detail::ObserveAll(executionContext.Observer, context, entityInfoProvider, entityPostOp);
	}
}}}
