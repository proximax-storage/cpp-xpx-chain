/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	class MultisigAccountFacade {
	public:
		explicit MultisigAccountFacade(cache::MultisigCacheDelta& multisigCache, const Key& multisigAccountKey)
				: m_multisigCache(multisigCache)
				, m_multisigAccountKey(multisigAccountKey)
				, m_multisigIter(GetMultisigEntry(m_multisigCache, m_multisigAccountKey))
				, m_multisigEntry(m_multisigIter.get())
		{}

		~MultisigAccountFacade() {
			removeIfEmpty(m_multisigEntry, m_multisigAccountKey);
		}

	public:
		void addCosignatory(const Key& cosignatoryKey) {
			auto multisigIter = GetMultisigEntry(m_multisigCache, cosignatoryKey);
			multisigIter.get().multisigAccounts().insert(m_multisigAccountKey);
			m_multisigEntry.cosignatories().insert(cosignatoryKey);
		}

		void removeCosignatory(const Key& cosignatoryKey) {
			m_multisigEntry.cosignatories().erase(cosignatoryKey);

			auto multisigIter = m_multisigCache.find(cosignatoryKey);
			auto& cosignatoryEntry = multisigIter.get();
			cosignatoryEntry.multisigAccounts().erase(m_multisigAccountKey);

			removeIfEmpty(cosignatoryEntry, cosignatoryKey);
		}

		void removeCosignatories() {
			auto cosignatories = m_multisigEntry.cosignatories();
			for (const auto& cosignatoryKey : cosignatories) {
				removeCosignatory(cosignatoryKey);
			}
		}

	private:
		void removeIfEmpty(const state::MultisigEntry& entry, const Key& key) {
			if (entry.cosignatories().empty() && entry.multisigAccounts().empty())
				m_multisigCache.remove(key);
		}

		cache::MultisigCacheDelta::iterator GetMultisigEntry(cache::MultisigCacheDelta& multisigCache, const Key& key) {
			if (!multisigCache.contains(key))
				multisigCache.insert(state::MultisigEntry(key));

			return multisigCache.find(key);
		}

	private:
		cache::MultisigCacheDelta& m_multisigCache;
		const Key& m_multisigAccountKey;
		cache::MultisigCacheDelta::iterator m_multisigIter;
		state::MultisigEntry& m_multisigEntry;
	};
}}
