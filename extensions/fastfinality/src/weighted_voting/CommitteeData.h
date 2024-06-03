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
#include "catapult/plugins/PluginManager.h"
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
		CommitteeData(const plugins::PluginManager& pluginManager)
			: m_pluginManager(pluginManager)
			, m_round(CommitteeRound{})
			, m_pBlockProposer(nullptr)
			, m_totalSumOfVotes{}
			, m_sumOfPrevotesSufficient(false)
			, m_sumOfPrecommitsSufficient(false)
			, m_unexpectedProposedBlockHeight(false)
			, m_unexpectedConfirmedBlockHeight(false)
			, m_isBlockBroadcastEnabled(false)
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

		void setProposedBlock(const std::shared_ptr<model::Block>& pBlock) {
			std::atomic_store(&m_pProposedBlock, pBlock);
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_proposedBlockHash = pBlock ? model::CalculateHash(*pBlock) : Hash256{};
			}
		}

		auto proposedBlock() const {
			return std::atomic_load(&m_pProposedBlock);
		}

		auto proposedBlockHash() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_proposedBlockHash;
		}

		std::future<bool> startWaitForConfirmedBlock() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_pConfirmedBlockPromise = std::make_shared<std::promise<bool>>();
			return m_pConfirmedBlockPromise->get_future();
		}

		void setConfirmedBlock(std::shared_ptr<model::Block> pBlock) {
			std::atomic_store(&m_pConfirmedBlock, std::move(pBlock));
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				if (m_pConfirmedBlockPromise) {
					m_pConfirmedBlockPromise->set_value(!!m_pConfirmedBlock);
					m_pConfirmedBlockPromise = nullptr;
				}
			}
		}

		auto confirmedBlock() const {
			return std::atomic_load(&m_pConfirmedBlock);
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

		std::future<bool> startWaitForPrevotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_pPrevotesPromise = std::make_shared<std::promise<bool>>();
			return m_pPrevotesPromise->get_future();
		}

		std::future<bool> startWaitForPrecommits() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_pPrecommitsPromise = std::make_shared<std::promise<bool>>();
			return m_pPrecommitsPromise->get_future();
		}

		void addVote(const CommitteeMessage& message) {
			std::lock_guard<std::mutex> guard(m_mutex);
			const auto& signer = message.BlockCosignature.Signer;
			if (message.Type == CommitteeMessageType::Prevote) {
				if ((m_prevotes.find(signer) == m_prevotes.end())) {
					m_prevotes.emplace(signer, message);
					if (isSumOfVotesSufficient(m_prevotes)) {
						m_sumOfPrevotesSufficient = true;
						if (m_pPrevotesPromise) {
							m_pPrevotesPromise->set_value(true);
							m_pPrevotesPromise = nullptr;
						}
					}
				}
			} else {
				m_precommitsToBroadcast.erase(signer);
				if (m_precommits.find(signer) == m_precommits.end()) {
					m_precommits.emplace(signer, message);
					if (m_precommitsToBroadcast.empty() && isSumOfVotesSufficient(m_precommits)) {
						m_sumOfPrecommitsSufficient = true;
						if (m_pPrecommitsPromise) {
							m_pPrecommitsPromise->set_value(true);
							m_pPrecommitsPromise = nullptr;
						}
					}
				}
			}
		}

		void calculateSumOfVotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_sumOfPrevotesSufficient = isSumOfVotesSufficient(m_prevotes);
			m_sumOfPrecommitsSufficient = isSumOfVotesSufficient(m_precommits);
		}

		void addVoteToBroadcast(const CommitteeMessage& message) {
			if (message.Type == CommitteeMessageType::Precommit) {
				std::lock_guard<std::mutex> guard(m_mutex);
				const auto& signer = message.BlockCosignature.Signer;
				if (m_precommitsToBroadcast.find(signer) == m_precommitsToBroadcast.end())
					m_precommitsToBroadcast.emplace(signer, message);
			}
		}

		void clearVotes() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_prevotes.clear();
			if (m_pPrevotesPromise) {
				m_pPrevotesPromise->set_value(false);
				m_pPrevotesPromise = nullptr;
			}

			m_precommits.clear();
			m_precommitsToBroadcast.clear();
			if (m_pPrecommitsPromise) {
				m_pPrecommitsPromise->set_value(false);
				m_pPrecommitsPromise = nullptr;
			}
		}

		void setSumOfPrevotesSufficient(bool value) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_sumOfPrevotesSufficient = value;
		}

		bool sumOfPrevotesSufficient() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_sumOfPrevotesSufficient;
		}

		void setSumOfPrecommitsSufficient(bool value) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_sumOfPrecommitsSufficient = value;
		}

		bool sumOfPrecommitsSufficient() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_sumOfPrecommitsSufficient;
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

		void setUnexpectedProposedBlockHeight(bool value) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_unexpectedProposedBlockHeight = value;
		}

		bool unexpectedProposedBlockHeight() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_unexpectedProposedBlockHeight;
		}

		void setUnexpectedConfirmedBlockHeight(bool value) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_unexpectedConfirmedBlockHeight = value;
		}

		bool unexpectedConfirmedBlockHeight() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_unexpectedConfirmedBlockHeight;
		}

		void addCommitteeBootKey(const Key& key) {
			if (key != Key())
				m_committeeBootKeys.emplace(key);
		}

		const std::set<Key>& committeeBootKeys() const {
			return m_committeeBootKeys;
		}

		void removeBootKeys() {
			m_committeeBootKeys.clear();
		}

		bool isProposedBlockValidated(const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			return (m_validatedProposedBlockSignatures.find(signature) != m_validatedProposedBlockSignatures.cend());
		}

		void addValidatedProposedBlockSignature(const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_validatedProposedBlockSignatures.emplace(signature);
		}

		void removeValidatedProposedBlockSignatures() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_validatedProposedBlockSignatures.clear();
		}

		bool isConfirmedBlockValidated(const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			return (m_validatedConfirmedBlockSignatures.find(signature) != m_validatedConfirmedBlockSignatures.cend());
		}

		void addValidatedConfirmedBlockSignature(const Signature& signature) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_validatedConfirmedBlockSignatures.emplace(signature);
		}

		void removeValidatedConfirmedBlockSignatures() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_validatedConfirmedBlockSignatures.clear();
		}

		void setIsBlockBroadcastEnabled(bool value) {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_isBlockBroadcastEnabled = value;
		}

		bool isBlockBroadcastEnabled() const {
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_isBlockBroadcastEnabled;
		}

	private:
		bool isSumOfVotesSufficient(const VoteMap& votes) {
			const auto& committeeManager = m_pluginManager.getCommitteeManager(Block_Version);
			const auto& config = m_pluginManager.configHolder()->Config(m_currentBlockHeight).Network;
			auto actualSumOfVotes = committeeManager.zeroWeight();
			for (const auto& pair : votes)
				committeeManager.add(actualSumOfVotes, committeeManager.weight(pair.first, config));

			auto minSumOfVotes = m_totalSumOfVotes;
			committeeManager.mul(minSumOfVotes, config.CommitteeApproval);
			auto sumOfVotesSufficient = committeeManager.ge(actualSumOfVotes, minSumOfVotes);
			CATAPULT_LOG(debug) << "sum of votes " << (sumOfVotesSufficient ? "" : "IN") << "SUFFICIENT [" << committeeManager.str(actualSumOfVotes) << ", " << committeeManager.str(minSumOfVotes) << "], vote count " << votes.size();
			return sumOfVotesSufficient;
		}

	private:
		static constexpr VersionType Block_Version = 5;

		const plugins::PluginManager& m_pluginManager;

		Key m_beneficiary;
		std::shared_ptr<harvesting::UnlockedAccounts> m_pUnlockedAccounts;

		std::atomic<CommitteeRound> m_round;

		const crypto::KeyPair* m_pBlockProposer;
		std::set<const crypto::KeyPair*> m_localCommittee;
		chain::HarvesterWeight m_totalSumOfVotes;

		std::shared_ptr<model::Block> m_pProposedBlock;
		Hash256 m_proposedBlockHash;

		std::shared_ptr<std::promise<bool>> m_pConfirmedBlockPromise;
		std::shared_ptr<model::Block> m_pConfirmedBlock;

		VoteMap m_prevotes;
		bool m_sumOfPrevotesSufficient;
		VoteMap m_precommits;
		bool m_sumOfPrecommitsSufficient;
		VoteMap m_precommitsToBroadcast;

		mutable std::mutex m_mutex;

		Height m_currentBlockHeight;

		bool m_unexpectedProposedBlockHeight;
		bool m_unexpectedConfirmedBlockHeight;
		std::set<Key> m_committeeBootKeys;

		std::shared_ptr<std::promise<bool>> m_pPrevotesPromise;
		std::shared_ptr<std::promise<bool>> m_pPrecommitsPromise;

		std::unordered_set<Signature, utils::ArrayHasher<Signature>> m_validatedProposedBlockSignatures;
		std::unordered_set<Signature, utils::ArrayHasher<Signature>> m_validatedConfirmedBlockSignatures;

		bool m_isBlockBroadcastEnabled;
	};
}}