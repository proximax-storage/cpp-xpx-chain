/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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

		// Increases the number of positive interactions of the account.
		void decrementPositiveInteractions() {
			if (m_nPositiveInteractions.unwrap() > 0)
				m_nPositiveInteractions = m_nPositiveInteractions - Reputation{1};
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

		// Increases the number of negative interactions of the account.
		void decrementNegativeInteractions() {
			if (m_nNegativeInteractions.unwrap() > 0)
				m_nNegativeInteractions = m_nNegativeInteractions - Reputation{1};
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
