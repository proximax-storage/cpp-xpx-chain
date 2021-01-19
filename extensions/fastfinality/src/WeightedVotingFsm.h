/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeData.h"
#include "WeightedVotingTransitionTable.h"
#include "catapult/thread/IoThreadPool.h"

namespace catapult { namespace ionet { class NodePacketIoPair; } }

namespace catapult { namespace fastfinality {

#define TRY_GET_FSM() \
		auto pFsmShared = pFsmWeak.lock(); \
		if (!pFsmShared) \
            return;

	struct ChainSyncData {
		Height NetworkHeight = Height(0);
		Height LocalHeight = Height(0);
		std::vector<Key> NodeIdentityKeys;
	};

	class WeightedVotingFsm {
	public:
		explicit WeightedVotingFsm(std::shared_ptr<thread::IoThreadPool> pPool)
			: m_pPool(std::move(pPool))
			, m_timer(m_pPool->ioContext())
			, m_sm(boost::sml::sm<WeightedVotingTransitionTable>(m_actions))
			, m_stopped(false)
		{}

	public:
		void start() {
			processEvent(StartLocalChainCheck{});
		}

		template<typename TEvent>
		void processEvent(const TEvent& event) {
			m_sm.process_event(event);
		}

		void shutdown() {
			m_stopped = true;
			m_timer.cancel();
			processEvent(Stop{});
			m_actions = WeightedVotingActions{};
			m_pPool = nullptr;
		}

		auto& ioContext() {
			return m_pPool->ioContext();
		}

		auto& timer() {
			return m_timer;
		}

		auto& actions() {
			return m_actions;
		}

		auto& chainSyncData() {
			if (!m_pChainSyncData)
				m_pChainSyncData = std::make_unique<ChainSyncData>();

			return *m_pChainSyncData;
		}

		void resetChainSyncData() {
			m_pChainSyncData.reset();
		}

		auto& committeeData() {
			return m_committeeData;
		}

		void resetCommitteeData() {
			m_committeeData.setCommitteeStage(CommitteeStage{});
			m_committeeData.setBlockProposer(nullptr);
			m_committeeData.localCommittee().clear();
			m_committeeData.setTotalSumOfVotes(0.0);
			m_committeeData.setProposedBlock(nullptr);
			m_committeeData.setConfirmedBlock(nullptr);
			m_committeeData.clearVotes();
		}

		bool stopped() const {
			return m_stopped;
		}

	private:
		std::shared_ptr<thread::IoThreadPool> m_pPool;
		boost::asio::system_timer m_timer;
		WeightedVotingActions m_actions;
		boost::sml::sm<WeightedVotingTransitionTable> m_sm;
		std::unique_ptr<ChainSyncData> m_pChainSyncData;
		CommitteeData m_committeeData;
		bool m_stopped;
	};
}}