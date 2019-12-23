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
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/utils/Hashers.h"
#include <vector>

namespace catapult { namespace harvesting {

#define UNLOCKED_ACCOUNTS_ADD_RESULT_LIST \
	/* Account was successfully unlocked (it might have already been unlocked). */ \
	ENUM_VALUE(Success) \
	\
	/* Account could not be unlocked because it is ineligible for harvesting. */ \
	ENUM_VALUE(Failure_Harvesting_Ineligible) \
	\
	/* Account could not be unlocked because it is blocked from harvesting. */ \
	ENUM_VALUE(Failure_Harvesting_Blocked) \
	\
	/* Account could not be unlocked because limit on the server has been hit. */ \
	ENUM_VALUE(Failure_Server_Limit)

#define ENUM_VALUE(LABEL) LABEL,
	/// Possible results of an add (unlock) operation.
	enum class UnlockedAccountsAddResult {
		UNLOCKED_ACCOUNTS_ADD_RESULT_LIST
	};
#undef ENUM_VALUE

	/// Insertion operator for outputting \a value to \a out.
	std::ostream& operator<<(std::ostream& out, UnlockedAccountsAddResult value);

	/// A read only view on top of unlocked accounts.
	class UnlockedAccountsView : utils::MoveOnly {
	public:
		/// Creates a view around \a keyPairs with lock context \a readLock.
		explicit UnlockedAccountsView(
				const std::vector<crypto::KeyPair>& keyPairs,
				utils::SpinReaderWriterLock::ReaderLockGuard&& readLock)
				: m_keyPairs(keyPairs)
				, m_readLock(std::move(readLock))
		{}

	public:
		/// Returns the number of unlocked accounts.
		size_t size() const;

		/// Returns \c true if the public key belongs to an unlocked account, \c false otherwise.
		bool contains(const Key& publicKey) const;

		/// Returns a const iterator to the first element of the underlying container.
		auto begin() const {
			return m_keyPairs.cbegin();
		}

		/// Returns a const iterator to the element following the last element of the underlying container.
		auto end() const {
			return m_keyPairs.cend();
		}

	private:
		const std::vector<crypto::KeyPair>& m_keyPairs;
		utils::SpinReaderWriterLock::ReaderLockGuard m_readLock;
	};

	/// A write only view on top of unlocked accounts.
	class UnlockedAccountsModifier : utils::MoveOnly {
	private:
		using KeyPredicate = predicate<const Key&>;

	public:
		/// Creates a view around \a maxUnlockedAccounts and \a keyPairs with lock context \a readLock.
		UnlockedAccountsModifier(
				size_t maxUnlockedAccounts,
				std::vector<crypto::KeyPair>& keyPairs,
				utils::SpinReaderWriterLock::WriterLockGuard&& writeLock)
				: m_maxUnlockedAccounts(maxUnlockedAccounts)
				, m_keyPairs(keyPairs)
                , m_writeLock(std::move(writeLock))
		{}

	public:
		/// Adds (unlocks) the account identified by key pair (\a keyPair).
		UnlockedAccountsAddResult add(crypto::KeyPair&& keyPair);

		/// Removes (locks) the account identified by the public key (\a publicKey).
		void remove(const Key& publicKey);

		/// Removes all accounts for which \a predicate returns \c true.
		void removeIf(const KeyPredicate& predicate);

	private:
		size_t m_maxUnlockedAccounts;
		std::vector<crypto::KeyPair>& m_keyPairs;
		utils::SpinReaderWriterLock::WriterLockGuard m_writeLock;
	};

	/// Container of all unlocked (harvesting candidate) accounts.
	class UnlockedAccounts {
	public:
		/// Creates an unlocked accounts container that allows at most \a maxUnlockedAccounts unloced accounts.
		explicit UnlockedAccounts(size_t maxUnlockedAccounts) : m_maxUnlockedAccounts(maxUnlockedAccounts)
		{}

	public:
		/// Gets a read only view of the unlocked accounts.
		UnlockedAccountsView view() const;

		/// Gets a write only view of the unlocked accounts.
		UnlockedAccountsModifier modifier();

	private:
		size_t m_maxUnlockedAccounts;
		std::vector<crypto::KeyPair> m_keyPairs;
		mutable utils::SpinReaderWriterLock m_lock;
	};
}}
