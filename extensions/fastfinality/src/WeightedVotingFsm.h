/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeData.h"
#include "WeightedVotingTransitionTable.h"
#include "dbrb/DbrbProcess.h"
#include "catapult/thread/IoThreadPool.h"

namespace catapult { namespace ionet { class NodePacketIoPair; } }

namespace catapult { namespace fastfinality {

#define TRY_GET_FSM() \
		auto pFsmShared = pFsmWeak.lock(); \
		if (!pFsmShared || pFsmShared->stopped()) \
            return;

	struct ChainSyncData {
		Height NetworkHeight = Height(0);
		Height LocalHeight = Height(0);
		std::vector<Key> NodeIdentityKeys;
	};

	class WeightedVotingFsm : public std::enable_shared_from_this<WeightedVotingFsm> {
	public:
		explicit WeightedVotingFsm(
				std::shared_ptr<thread::IoThreadPool> pPool,
				const config::BlockchainConfiguration& config,
				std::shared_ptr<dbrb::DbrbProcess> pDbrbProcess)
			: m_pPool(std::move(pPool))
			, m_timer(m_pPool->ioContext())
			, m_sm(boost::sml::sm<WeightedVotingTransitionTable>(m_actions))
			, m_strand(m_pPool->ioContext())
			, m_nodeWorkState(NodeWorkState::None)
			, m_stopped(false)
			, m_pDbrbProcess(std::move(pDbrbProcess))
			, m_packetHandlers(config.Node.MaxPacketDataSize.bytes32())
		{}

	public:
		void start() {
			processEvent(StartLocalChainCheck{});
		}

		template<typename TEvent>
		void processEvent(const TEvent& event) {
			auto func = __PRETTY_FUNCTION__;
			boost::asio::post(m_strand, [pThisWeak = weak_from_this(), event, func] {
				auto pThis = pThisWeak.lock();
				if (!pThis)
					return;

				CATAPULT_LOG(debug) << func;
				pThis->m_sm.process_event(event);
			});
		}

		void shutdown() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_stopped = true;
			m_timer.cancel();
			processEvent(Stop{});
			m_actions = WeightedVotingActions{};
			m_pPool = nullptr;
			m_pDbrbProcess = nullptr;
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

		const auto& committeeData() const {
			return m_committeeData;
		}

		void resetCommitteeData() {
			m_committeeData.setCommitteeRound(CommitteeRound{});
			m_committeeData.setBlockProposer(nullptr);
			m_committeeData.localCommittee().clear();
			m_committeeData.setTotalSumOfVotes(chain::HarvesterWeight{});
			m_committeeData.setProposedBlock(nullptr);
			m_committeeData.setConfirmedBlock(nullptr);
			m_committeeData.clearVotes();
			m_committeeData.setSumOfPrevotesSufficient(false);
			m_committeeData.stopWaitForProposedBlock();
		}

		bool stopped() const {
			return m_stopped;
		}

		auto nodeWorkState() {
			return m_nodeWorkState;
		}

		void setNodeWorkState(NodeWorkState state) {
			m_nodeWorkState = state;
		}

		auto& mutex() {
			return m_mutex;
		}

		const auto& dbrbProcess() {
			return m_pDbrbProcess;
		}

		auto& packetHandlers() {
			return m_packetHandlers;
		}

	private:
		std::shared_ptr<thread::IoThreadPool> m_pPool;
		boost::asio::system_timer m_timer;
		WeightedVotingActions m_actions;
		boost::sml::sm<WeightedVotingTransitionTable> m_sm;
		std::unique_ptr<ChainSyncData> m_pChainSyncData;
		CommitteeData m_committeeData;
		boost::asio::io_context::strand m_strand;
		NodeWorkState m_nodeWorkState;
		bool m_stopped;
		mutable std::mutex m_mutex;
		std::shared_ptr<dbrb::DbrbProcess> m_pDbrbProcess;
		ionet::ServerPacketHandlers m_packetHandlers;
	};
}}