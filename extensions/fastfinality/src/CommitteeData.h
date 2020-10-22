/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeStage.h"
#include "catapult/model/EntityHasher.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace harvesting { class UnlockedAccounts; }
	namespace model { class Block; }
}

namespace catapult { namespace fastfinality {

	using VoteMap = std::map<Key, Signature>;

	class CommitteeData {
	public:
		CommitteeData()
			: m_stage(CommitteeStage{})
			, m_pBlockProposer(nullptr)
			, m_totalSumOfVotes(0.0)
			, m_proposalMultiple(false)
		{}

	public:
		void setBeneficiary(const Key& key) {
			m_beneficiary = key;
		}

		const auto& beneficiary() const {
			return m_beneficiary;
		}

		void setUnlockedAccounts(const std::shared_ptr<harvesting::UnlockedAccounts>& pUnlockedAccounts) {
			m_pUnlockedAccounts = pUnlockedAccounts;
		}

		const auto& unlockedAccounts() const {
			return m_pUnlockedAccounts;
		}

		void setCommitteeStage(const CommitteeStage& stage) {
			m_stage = stage;
		}

		auto committeeStage() const {
			return m_stage.load();
		}

		void setBlockProposer(const crypto::KeyPair* pBlockProposer) {
			m_pBlockProposer = pBlockProposer;
		}

		const auto* blockProposer() const {
			return m_pBlockProposer;
		}

		const auto& localCommittee() const {
			return m_localCommittee;
		}

		auto& localCommittee() {
			return m_localCommittee;
		}

		void setTotalSumOfVotes(double sum) {
			m_totalSumOfVotes = sum;
		}

		auto totalSumOfVotes() const {
			return m_totalSumOfVotes;
		}

		void setProposedBlock(std::shared_ptr<model::Block> pBlock) {
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_proposedBlockHash = !!pBlock ? model::CalculateHash(*pBlock) : Hash256{};
			}
			std::atomic_store(&m_pProposedBlock, pBlock);
		}

		auto proposedBlock() const {
			return std::atomic_load(&m_pProposedBlock);
		}

		void setProposalMultiple(bool value) {
			m_proposalMultiple = value;
		}

		auto proposalMultiple() const {
			return m_proposalMultiple.load();
		}

		auto proposedBlockHash() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_proposedBlockHash;
		}

		void addPrevote(const Key& signer, const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.emplace(signer, signature);
		}

		void clearPrevotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.clear();
		}

		auto prevotes() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_prevotes;
		}

		auto getPrevote(const Key& signer) {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_prevotes.at(signer);
		}

		void addPrecommit(const Key& signer, const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_precommits.emplace(signer, signature);
		}

		void clearPrecommits() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_precommits.clear();
		}

		auto precommits() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_precommits;
		}

	private:
		Key m_beneficiary;
		std::shared_ptr<harvesting::UnlockedAccounts> m_pUnlockedAccounts;

		std::atomic<CommitteeStage> m_stage;

		const crypto::KeyPair* m_pBlockProposer;
		std::set<const crypto::KeyPair*> m_localCommittee;
		double m_totalSumOfVotes;

		std::shared_ptr<model::Block> m_pProposedBlock;
		std::atomic_bool m_proposalMultiple;
		Hash256 m_proposedBlockHash;

		VoteMap m_prevotes;
		VoteMap m_precommits;

		mutable std::mutex m_mutex;
	};
}}