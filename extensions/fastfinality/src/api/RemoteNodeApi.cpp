/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoteNodeApi.h"
#include "catapult/api/RemoteRequestDispatcher.h"
#include "catapult/model/RangeTypes.h"

namespace catapult { namespace fastfinality {

	namespace {
		// region traits

		template<typename NameTraits, ionet::PacketType PacketType>
		struct BlockTraits : public api::RegistryDependentTraits<model::Block> {
		public:
			using ResultType = std::shared_ptr<model::Block>;
			static constexpr auto Packet_Type = PacketType;
			static constexpr auto Friendly_Name = NameTraits::Friendly_Name;

			static auto CreateRequestPacketPayload() {
				return ionet::PacketPayload(PacketType);
			}

		public:
			using RegistryDependentTraits::RegistryDependentTraits;

			bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
				auto range = ionet::ExtractEntitiesFromPacket<model::Block>(packet, *this);
				if (!range.empty())
					result = model::BlockRange::ExtractEntitiesFromRange(std::move(range))[0];
				return !!result || sizeof(ionet::PacketHeader) == packet.Size;
			}
		};

		struct ProposedBlockNameTraits {
			static constexpr auto Friendly_Name = "pull proposed block";
		};

		struct ConfirmedBlockNameTraits {
			static constexpr auto Friendly_Name = "pull confirmed block";
		};
		
		using ProposedBlockTraits = BlockTraits<ProposedBlockNameTraits, ionet::PacketType::Pull_Proposed_Block>;
		using ConfirmedBlockTraits = BlockTraits<ConfirmedBlockNameTraits, ionet::PacketType::Pull_Confirmed_Block>;

		template<typename NameTraits, typename TPacket>
		struct CommitteeMessagesTraits {
		public:
			using ResultType = std::vector<CommitteeMessage>;
			static constexpr auto Packet_Type = TPacket::Packet_Type;
			static constexpr auto Friendly_Name = NameTraits::Friendly_Name;

			static auto CreateRequestPacketPayload() {
				return ionet::PacketPayload(TPacket::Packet_Type);
			}

		public:
			bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
				auto pPacket = static_cast<const TPacket*>(&packet);
				if (!pPacket)
					return false;

				const auto* pMessage = reinterpret_cast<const CommitteeMessage*>(pPacket + 1);
				for (uint8_t i = 0u; i < pPacket->MessageCount; ++i, ++pMessage)
					result.push_back(*pMessage);

				return true;
			}
		};

		struct PrevoteMessagesNameTraits {
			static constexpr auto Friendly_Name = "pull prevote messages";
		};

		struct PrecommitMessagesNameTraits {
			static constexpr auto Friendly_Name = "pull precommit messages";
		};

		using PrevoteMessagesTraits = CommitteeMessagesTraits<PrevoteMessagesNameTraits, PullPrevoteMessagesRequest>;
		using PrecommitMessagesTraits = CommitteeMessagesTraits<PrecommitMessagesNameTraits, PullPrecommitMessagesRequest>;

		// endregion

		class DefaultRemoteNodeApi : public RemoteNodeApi {
		private:
			template<typename TTraits>
			using FutureType = thread::future<typename TTraits::ResultType>;

		public:
			DefaultRemoteNodeApi(ionet::PacketIo& io, const model::TransactionRegistry* pRegistry)
					: m_pRegistry(pRegistry)
					, m_impl(io)
			{}

		public:
			FutureType<ProposedBlockTraits> proposedBlock() const override {
				return m_impl.dispatch(ProposedBlockTraits(*m_pRegistry));
			}

			FutureType<ConfirmedBlockTraits> confirmedBlock() const override {
				return m_impl.dispatch(ConfirmedBlockTraits(*m_pRegistry));
			}

			FutureType<PrevoteMessagesTraits> prevotes() const override {
				return m_impl.dispatch(PrevoteMessagesTraits());
			}

			FutureType<PrecommitMessagesTraits> precommits() const override {
				return m_impl.dispatch(PrecommitMessagesTraits());
			}

		private:
			const model::TransactionRegistry* m_pRegistry;
			mutable api::RemoteRequestDispatcher m_impl;
		};
	}

	std::unique_ptr<RemoteNodeApi> CreateRemoteNodeApi(ionet::PacketIo& io, const model::TransactionRegistry& registry) {
		return std::make_unique<DefaultRemoteNodeApi>(io, &registry);
	}

	std::unique_ptr<RemoteNodeApi> CreateRemoteNodeApiWithoutRegistry(ionet::PacketIo& io) {
		return std::make_unique<DefaultRemoteNodeApi>(io, nullptr);
	}
}}
