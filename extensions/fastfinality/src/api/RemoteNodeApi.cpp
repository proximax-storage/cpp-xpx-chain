/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoteNodeApi.h"
#include "catapult/api/RemoteApiUtils.h"
#include "catapult/api/RemoteRequestDispatcher.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/ionet/PacketPayloadFactory.h"
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

		struct ConfirmedBlockNameTraits {
			static constexpr auto Friendly_Name = "pull confirmed block";
		};

		using ConfirmedBlockTraits = BlockTraits<ConfirmedBlockNameTraits, ionet::PacketType::Pull_Confirmed_Block>;

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
			FutureType<ConfirmedBlockTraits> confirmedBlock() const override {
				return m_impl.dispatch(ConfirmedBlockTraits(*m_pRegistry));
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
