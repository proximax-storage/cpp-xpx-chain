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

namespace catapult { namespace state {

	// Mixin for storing account reputation.
	class ReputationMixin {
	public:
		// Gets the number of positive interactions of the account.
		Reputation positiveInteractions() const {
			return m_nPositiveInteractions;
		}

		// Sets the number of positive interactions of the account.
		void setPositiveInteractions(const Reputation& positiveInteractions) {
			m_nPositiveInteractions = positiveInteractions;
		}

		// Increases the number of positive interactions of the account.
		void incrementPositiveInteractions() {
			m_nPositiveInteractions = m_nPositiveInteractions + Reputation{1};
		}

		// Gets the number of negative interactions of the account.
		Reputation negativeInteractions() const {
			return m_nNegativeInteractions;
		}

		// Sets the number of negative interactions of the account.
		void setNegativeInteractions(const Reputation& negativeInteractions) {
			m_nNegativeInteractions = negativeInteractions;
		}

		// Increases the number of negative interactions of the account.
		void incrementNegativeInteractions() {
			m_nNegativeInteractions = m_nNegativeInteractions + Reputation{1};
		}

	private:
		Reputation m_nPositiveInteractions;
		Reputation m_nNegativeInteractions;
	};

	// Reputation entry.
	class ReputationEntry : public ReputationMixin {
	public:
		// Creates a reputation entry around \a key.
		explicit ReputationEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the account public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
