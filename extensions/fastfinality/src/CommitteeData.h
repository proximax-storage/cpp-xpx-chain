/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeRound.h"
#include "WeightedVotingChainPackets.h"
#include "catapult/chain/CommitteeManager.h"
#include "catapult/model/EntityHasher.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <future>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace harvesting { class UnlockedAccounts; }
	namespace model { class Block; }
}

namespace catapult { namespace fastfinality {

	using VoteMap = std::map<Key, CommitteeMessage>;

	class CommitteeData {
	public:
		CommitteeData()
			: m_round(CommitteeRound{})
			, m_pBlockProposer(nullptr)
			, m_totalSumOfVotes{}
			, m_sumOfPrevotesSufficient(false)
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

		void setCommitteeRound(const CommitteeRound& round) {
			m_round = round;
		}

		auto committeeRound() const {
			return m_round.load();
		}

		void setBlockProposer(const crypto::KeyPair* pBlockProposer) {
			m_pBlockProposer = pBlockProposer;
		}

		const auto* blockProposer() const {
			return m_pBlockProposer;
		}

		auto& localCommittee() {
			return m_localCommittee;
		}

		void setTotalSumOfVotes(chain::HarvesterWeight sum) {
			m_totalSumOfVotes = sum;
		}

		auto totalSumOfVotes() const {
			return m_totalSumOfVotes;
		}

		std::future<bool> startWaitForProposedBlock() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_pProposedBlockPromise = std::make_shared<std::promise<bool>>();
			return m_pProposedBlockPromise->get_future();
		}

		void stopWaitForProposedBlock() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_pProposedBlockPromise = nullptr;
		}

		void setProposedBlock(const std::shared_ptr<model::Block>& pBlock) {
			std::atomic_store(&m_pProposedBlock, pBlock);
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_proposedBlockHash = pBlock ? model::CalculateHash(*pBlock) : Hash256{};
				if (m_pProposedBlockPromise) {
					m_pProposedBlockPromise->set_value(true);
					m_pProposedBlockPromise = nullptr; // TODO: test behaviour
				}
			}
		}

		auto proposedBlock() const {
			return std::atomic_load(&m_pProposedBlock);
		}

		auto proposedBlockHash() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_proposedBlockHash;
		}

		void setConfirmedBlock(std::shared_ptr<model::Block> pBlock) {
			std::atomic_store(&m_pConfirmedBlock, std::move(pBlock));
		}

		auto confirmedBlock() const {
			return std::atomic_load(&m_pConfirmedBlock);
		}

		auto prevotes() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_prevotes;
		}

		auto precommits() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_precommits;
		}

		auto votes(CommitteeMessageType type) const {
			std::lock_guard<std::mutex> guard(m_mutex);
			if (type == CommitteeMessageType::Prevote) {
				return m_prevotes;
			} else {
				return m_precommits;
			}
		}

		bool hasVote(const Key& signer, CommitteeMessageType type) {
			std::lock_guard<std::mutex> guard(m_mutex);
			if (type == CommitteeMessageType::Prevote) {
				return (m_prevotes.find(signer) != m_prevotes.end());
			} else {
				return (m_precommits.find(signer) != m_precommits.end());
			}
		}

		void addVote(const CommitteeMessage& message) {
			std::lock_guard<std::mutex> guard(m_mutex);
			const auto& signer = message.BlockCosignature.Signer;
			if (message.Type == CommitteeMessageType::Prevote) {
				if ((m_prevotes.find(signer) == m_prevotes.end()))
					m_prevotes.emplace(signer, message);
			} else {
				if (m_precommits.find(signer) == m_precommits.end())
					m_precommits.emplace(signer, message);
			}
		}

		void clearVotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.clear();
			m_precommits.clear();
		}

		void setSumOfPrevotesSufficient(bool value) {
			m_sumOfPrevotesSufficient = value;
		}

		bool sumOfPrevotesSufficient() const {
			return m_sumOfPrevotesSufficient;
		}

		void setCurrentBlockHeight(const Height& value) {
			m_currentBlockHeight = value;
		}

		void incrementCurrentBlockHeight() {
			m_currentBlockHeight = m_currentBlockHeight + Height(1);
		}

		auto currentBlockHeight() const {
			return m_currentBlockHeight;
		}

	private:
		Key m_beneficiary;
		std::shared_ptr<harvesting::UnlockedAccounts> m_pUnlockedAccounts;

		std::atomic<CommitteeRound> m_round;

		const crypto::KeyPair* m_pBlockProposer;
		std::set<const crypto::KeyPair*> m_localCommittee;
		chain::HarvesterWeight m_totalSumOfVotes;

		std::shared_ptr<std::promise<bool>> m_pProposedBlockPromise;
		std::shared_ptr<model::Block> m_pProposedBlock;
		Hash256 m_proposedBlockHash;
		std::shared_ptr<model::Block> m_pConfirmedBlock;

		VoteMap m_prevotes;
		VoteMap m_precommits;
		bool m_sumOfPrevotesSufficient;

		mutable std::mutex m_mutex;

		Height m_currentBlockHeight;
	};
}}