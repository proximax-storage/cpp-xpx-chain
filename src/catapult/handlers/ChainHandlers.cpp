/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "ChainHandlers.h"
#include "HandlerUtils.h"
#include "catapult/api/ChainPackets.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/Block.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace handlers {

	void RegisterPushBlockHandler(
			ionet::ServerPacketHandlers& handlers,
			const model::TransactionRegistry& registry,
			const BlockRangeHandler& blockRangeHandler) {
		handlers.registerHandler(ionet::PacketType::Push_Block, CreatePushEntityHandler<model::Block>(registry, blockRangeHandler));
	}

	namespace {
		template<typename TPacket>
		auto CreateResponsePacket(uint32_t payloadSize) {
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			pPacket->Type = TPacket::Packet_Type;
			return pPacket;
		}

		template<typename TRequest>
		struct HeightRequestInfo {
		public:
			constexpr HeightRequestInfo() : pRequest(nullptr)
			{}

		public:
			Height ChainHeight;
			const TRequest* pRequest;

		public:
			uint32_t numAvailableBlocks() const {
				return static_cast<uint32_t>((ChainHeight - pRequest->Height).unwrap() + 1);
			}
		};

		template<typename TRequest>
		HeightRequestInfo<TRequest> ProcessHeightRequest(
				const io::BlockStorageView& storage,
				const ionet::Packet& packet,
				ionet::ServerPacketHandlerContext& context,
				bool allowZeroHeight) {
			const auto* pRequest = ionet::CoercePacket<TRequest>(&packet);
			if (!pRequest)
				return HeightRequestInfo<TRequest>();

			HeightRequestInfo<TRequest> info;
			info.ChainHeight = storage.chainHeight();
			CATAPULT_LOG(trace) << "local height = " << info.ChainHeight << ", request height = " << pRequest->Height;
			if (info.ChainHeight < pRequest->Height || (!allowZeroHeight && Height(0) == pRequest->Height)) {
				context.response(ionet::PacketPayload(CreateResponsePacket<TRequest>(0)));
				return HeightRequestInfo<TRequest>();
			}

			info.pRequest = pRequest;
			return info;
		}

		auto CreatePullBlockHandler(const io::BlockStorageCache& storage) {
			return [&storage](const auto& packet, auto& context) {
				using RequestType = api::PullBlockRequest;
				auto storageView = storage.view();
				auto info = ProcessHeightRequest<RequestType>(storageView, packet, context, true);
				if (!info.pRequest)
					return;

				auto height = info.pRequest->Height;
				height = Height(0) == height ? info.ChainHeight : height;
				auto pBlock = storageView.loadBlock(height);

				auto payload = ionet::PacketPayloadFactory::FromEntity(RequestType::Packet_Type, std::move(pBlock));
				context.response(std::move(payload));
			};
		}
	}

	void RegisterPullBlockHandler(ionet::ServerPacketHandlers& handlers, const io::BlockStorageCache& storage) {
		handlers.registerHandler(ionet::PacketType::Pull_Block, CreatePullBlockHandler(storage));
	}

	namespace {
		auto CreateChainInfoHandler(const io::BlockStorageCache& storage) {
			return [&storage](const auto& packet, auto& context) {
				using RequestType = api::ChainInfoResponse;
				if (!ionet::IsPacketValid(packet, RequestType::Packet_Type))
					return;

				auto pResponsePacket = ionet::CreateSharedPacket<RequestType>();
				auto view = storage.view();
				pResponsePacket->Height = view.chainHeight();
				pResponsePacket->Difficulty = view.loadBlock(pResponsePacket->Height)->CumulativeDifficulty;
				context.response(ionet::PacketPayload(pResponsePacket));
			};
		}
	}

	void RegisterChainInfoHandler(
			ionet::ServerPacketHandlers& handlers,
			const io::BlockStorageCache& storage) {
		handlers.registerHandler(ionet::PacketType::Chain_Info, CreateChainInfoHandler(storage));
	}

	namespace {
		auto CreateBlockHashesHandler(const io::BlockStorageCache& storage, uint32_t maxHashes) {
			return [&storage, maxHashes](const auto& packet, auto& context) {
				using RequestType = api::BlockHashesRequest;
				auto storageView = storage.view();
				auto info = ProcessHeightRequest<RequestType>(storageView, packet, context, false);
				if (!info.pRequest)
					return;

				auto hashes = storageView.loadHashesFrom(info.pRequest->Height, maxHashes);
				auto payload = ionet::PacketPayloadFactory::FromFixedSizeRange(RequestType::Packet_Type, std::move(hashes));
				context.response(std::move(payload));
			};
		}
	}

	void RegisterBlockHashesHandler(ionet::ServerPacketHandlers& handlers, const io::BlockStorageCache& storage, uint32_t maxHashes) {
		handlers.registerHandler(ionet::PacketType::Block_Hashes, CreateBlockHashesHandler(storage, maxHashes));
	}

	namespace {
		uint32_t ClampNumBlocks(const HeightRequestInfo<api::PullBlocksRequest>& info, const PullBlocksHandlerConfiguration& config) {
			auto numBlocks = std::min(config.MaxBlocks, info.pRequest->NumBlocks);
			return std::min(numBlocks, info.numAvailableBlocks());
		}

		uint32_t ClampNumResponseBytes(
				const HeightRequestInfo<api::PullBlocksRequest>& info,
				const PullBlocksHandlerConfiguration& config) {
			return std::min(config.MaxResponseBytes, info.pRequest->NumResponseBytes);
		}

		auto CreatePullBlocksHandler(const io::BlockStorageCache& storage, const PullBlocksHandlerConfiguration& config) {
			return [&storage, config](const auto& packet, auto& context) {
				using RequestType = api::PullBlocksRequest;
				auto storageView = storage.view();
				auto info = ProcessHeightRequest<RequestType>(storageView, packet, context, false);
				if (!info.pRequest)
					return;

				auto numBlocks = ClampNumBlocks(info, config);
				auto numResponseBytes = ClampNumResponseBytes(info, config);

				uint32_t payloadSize = 0;
				std::vector<std::shared_ptr<const model::Block>> blocks;
				for (auto i = 0u; i < numBlocks; ++i) {
					// always return at least one block
					auto pBlock = storageView.loadBlock(info.pRequest->Height + Height(i));
					if (!blocks.empty() && payloadSize + pBlock->Size > numResponseBytes)
						break;

					payloadSize += pBlock->Size;
					blocks.push_back(std::move(pBlock));
				}

				auto payload = ionet::PacketPayloadFactory::FromEntities(RequestType::Packet_Type, blocks);
				context.response(std::move(payload));
			};
		}
	}

	void RegisterPullBlocksHandler(
			ionet::ServerPacketHandlers& handlers,
			const io::BlockStorageCache& storage,
			const PullBlocksHandlerConfiguration& config) {
		handlers.registerHandler(ionet::PacketType::Pull_Blocks, CreatePullBlocksHandler(storage, config));
	}
}}
