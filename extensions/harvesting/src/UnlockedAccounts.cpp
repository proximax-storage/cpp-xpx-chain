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

#include "UnlockedAccounts.h"
#include "catapult/utils/MacroBasedEnumIncludes.h"
#include "BlockGeneratorAccountDescriptor.h"
namespace catapult { namespace harvesting {

#define DEFINE_ENUM UnlockedAccountsAddResult
#define ENUM_LIST UNLOCKED_ACCOUNTS_ADD_RESULT_LIST
#include "catapult/utils/MacroBasedEnum.h"

#undef ENUM_LIST
#undef DEFINE_ENUM

	namespace {
		auto CreateContainsPredicate(const Key& publicKey) {
			return [&publicKey](const auto& descriptorContainer) {
				return std::get<PrioritizedContainerIdx::Descriptor>(descriptorContainer).signingKeyPair().publicKey() == publicKey;
			};
		}
	}

	// region UnlockedAccountsView

	size_t UnlockedAccountsView::size() const {
		return m_prioritizedKeyPairs.size();
	}

	bool UnlockedAccountsView::contains(const Key& publicKey) const {
		return std::any_of(m_prioritizedKeyPairs.cbegin(), m_prioritizedKeyPairs.cend(), CreateContainsPredicate(publicKey));
	}

	void UnlockedAccountsView::forEach(const predicate<const BlockGeneratorAccountDescriptor&>& consumer) const {
		for (const auto& prioritizedKeyPair : m_prioritizedKeyPairs) {
			if (!consumer(std::get<PrioritizedContainerIdx::Descriptor>(prioritizedKeyPair)))
				break;
		}
	}
	// endregion

	// region UnlockedAccountsModifier

	UnlockedAccountsAddResult UnlockedAccountsModifier::add(BlockGeneratorAccountDescriptor&& descriptor) {
		const auto& publicKey = descriptor.signingKeyPair().publicKey();
		for (auto& prioritizedKeyPair : m_prioritizedKeyPairs) {
			if (CreateContainsPredicate(publicKey)(prioritizedKeyPair)) {
				std::get<PrioritizedContainerIdx::Descriptor>(prioritizedKeyPair) = std::move(descriptor);
				return UnlockedAccountsAddResult::Success_Update;
			}
		}

		if (std::any_of(m_prioritizedKeyPairs.cbegin(), m_prioritizedKeyPairs.cend(), CreateContainsPredicate(publicKey)))
			return UnlockedAccountsAddResult::Success_Update;

		auto priorityScore = m_prioritizer(publicKey);
		if (m_maxUnlockedAccounts == m_prioritizedKeyPairs.size()) {
			if (std::get<PrioritizedContainerIdx::Priority>(m_prioritizedKeyPairs.back()) >= priorityScore)
				return UnlockedAccountsAddResult::Failure_Server_Limit;

			m_prioritizedKeyPairs.pop_back();
		}

		auto iter = m_prioritizedKeyPairs.cbegin();
		for (; m_prioritizedKeyPairs.cend() != iter; ++iter) {
			if (priorityScore > std::get<PrioritizedContainerIdx::Priority>(*iter))
				break;
		}
		m_prioritizedKeyPairs.emplace(iter, std::move(descriptor), priorityScore);
		return UnlockedAccountsAddResult::Success_New;
	}

	bool UnlockedAccountsModifier::remove(const Key& publicKey) {
		auto iter = std::remove_if(m_prioritizedKeyPairs.begin(), m_prioritizedKeyPairs.end(), CreateContainsPredicate(publicKey));
		if (m_prioritizedKeyPairs.end() != iter)
		{
			m_prioritizedKeyPairs.erase(iter);
			return true;
		}
		return false;

	}

	void UnlockedAccountsModifier::removeIf(const KeyPredicate& predicate) {
		auto initialSize = m_prioritizedKeyPairs.size();
		auto newKeyPairsEnd = std::remove_if(m_prioritizedKeyPairs.begin(), m_prioritizedKeyPairs.end(), [predicate](const auto& descriptorContainer) {
			return predicate(std::get<PrioritizedContainerIdx::Descriptor>(descriptorContainer));
		});

		m_prioritizedKeyPairs.erase(newKeyPairsEnd, m_prioritizedKeyPairs.end());
		if (m_prioritizedKeyPairs.size() != initialSize)
			CATAPULT_LOG(info) << "pruned " << (initialSize - m_prioritizedKeyPairs.size()) << " unlocked accounts";
	}

	// endregion

	// region UnlockedAccounts

	UnlockedAccountsView UnlockedAccounts::view() const {
		auto readLock = m_lock.acquireReader();
		return UnlockedAccountsView(m_prioritizedKeyPairs, std::move(readLock));
	}

	UnlockedAccountsModifier UnlockedAccounts::modifier() {
		auto writeLock  = m_lock.acquireWriter();
		return UnlockedAccountsModifier(m_maxUnlockedAccounts,m_prioritizedKeyPairs, m_prioritizer, std::move(writeLock));
	}

	// endregion
}}
