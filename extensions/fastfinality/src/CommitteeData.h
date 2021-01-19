/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeStage.h"
#include "WeightedVotingChainPackets.h"
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

	template<typename TPacket>
	using VoteMap = std::map<Key, std::shared_ptr<TPacket>>;

	class CommitteeData {
	public:
		CommitteeData()
			: m_stage(CommitteeStage{})
			, m_pBlockProposer(nullptr)
			, m_totalSumOfVotes(0.0)
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
				m_proposedBlockHash = pBlock ? model::CalculateHash(*pBlock) : Hash256{};
			}
			std::atomic_store(&m_pProposedBlock, pBlock);
		}

		auto proposedBlock() const {
			return std::atomic_load(&m_pProposedBlock);
		}

		void setConfirmedBlock(std::shared_ptr<model::Block> pBlock) {
			std::atomic_store(&m_pConfirmedBlock, pBlock);
		}

		auto confirmedBlock() const {
			return std::atomic_load(&m_pConfirmedBlock);
		}

		auto proposedBlockHash() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_proposedBlockHash;
		}

		auto prevotes() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_prevotes;
		}

		auto precommits() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_precommits;
		}

		void addVote(std::shared_ptr<PrevoteMessagePacket>&& pPacket) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.emplace(pPacket->Message.BlockCosignature.Signer, std::move(pPacket));
		}

		void addVote(std::shared_ptr<PrecommitMessagePacket>&& pPacket) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_precommits.emplace(pPacket->Message.BlockCosignature.Signer, std::move(pPacket));
		}

		void clearVotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.clear();
			m_precommits.clear();
		}

		bool hasVote(const Key& signer, CommitteeMessageType type) {
			std::lock_guard<std::mutex> guard(m_mutex);
			switch (type) {
				case CommitteeMessageType::Prevote:
					return (m_prevotes.find(signer) != m_prevotes.end());

				case CommitteeMessageType::Precommit:
					return (m_precommits.find(signer) != m_precommits.end());

				default:
					return false;
			}
		}

	private:
		Key m_beneficiary;
		std::shared_ptr<harvesting::UnlockedAccounts> m_pUnlockedAccounts;

		std::atomic<CommitteeStage> m_stage;

		const crypto::KeyPair* m_pBlockProposer;
		std::set<const crypto::KeyPair*> m_localCommittee;
		double m_totalSumOfVotes;

		std::shared_ptr<model::Block> m_pProposedBlock;
		Hash256 m_proposedBlockHash;
		std::shared_ptr<model::Block> m_pConfirmedBlock;

		VoteMap<PrevoteMessagePacket> m_prevotes;
		VoteMap<PrecommitMessagePacket> m_precommits;
		mutable std::mutex m_mutex;
	};
}}