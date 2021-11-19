/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StoragePacketHandlers.h"
#include "ReplicatorService.h"
#include "StorageChainPackets.h"
#include "catapult/ionet/PacketHandlers.h"

namespace catapult { namespace storage {

	namespace {
		using FileHandler = std::function<FileNames (std::shared_ptr<ReplicatorService> pReplicatorService, const Key&, const Key&, FileNames&&)>;

		template<ionet::PacketType RequestPacketType, ionet::PacketType ResponsePacketType>
		void RegisterFileDownloadHandler(
				const std::weak_ptr<ReplicatorService>& pReplicatorServiceWeak,
				ionet::ServerPacketHandlers& handlers,
				const FileHandler& fileHandler) {
			handlers.registerHandler(RequestPacketType, [pReplicatorServiceWeak, fileHandler](
					const auto& packet, auto& context) {
				auto pReplicatorService = pReplicatorServiceWeak.lock();
				if (!pReplicatorService)
					return;

				const auto pRequest = ionet::CoercePacket<FileDownloadPacket<RequestPacketType>>(&packet);
				if (!pRequest) {
					CATAPULT_LOG(warning) << "rejecting empty request: " << packet;
					return;
				}

				CATAPULT_LOG(trace) << "received valid " << packet;

				FileNames fileNames;
				fileNames.reserve(pRequest->FileCount);
				const auto* pRequestPayload = reinterpret_cast<const char*>(pRequest + 1);
				for (auto i = 0u; i < pRequest->FileCount; ++i) {
					auto fileNameSize = *reinterpret_cast<const uint16_t*>(pRequestPayload);
					pRequestPayload += sizeof(uint16_t);
					std::string fileName(pRequestPayload, fileNameSize);
					pRequestPayload += fileNameSize;
					fileNames.push_back(fileName);
				}

				auto failedFiles = fileHandler(pReplicatorService, context.key(), pRequest->DriveKey, std::move(fileNames));
				uint32_t responsePayloadSize = 0u;
				for (const auto& fileName : failedFiles)
					responsePayloadSize += sizeof(uint16_t) + fileName.size();
				auto pResponse = ionet::CreateSharedPacket<storage::FileDownloadPacket<ResponsePacketType>>(responsePayloadSize);
				pResponse->DriveKey = pRequest->DriveKey;
				pResponse->FileCount = utils::checked_cast<size_t, uint32_t>(failedFiles.size());
				auto* pResponsePayload = reinterpret_cast<char*>(pResponse.get() + 1);

				for (const auto& fileName : failedFiles) {
					auto fileNameSize = fileName.size();
					*reinterpret_cast<uint16_t*>(pResponsePayload) = utils::checked_cast<size_t, uint16_t>(fileNameSize);
					pResponsePayload += sizeof(uint16_t);
					memcpy(pResponsePayload, fileName.data(), fileNameSize);
					pResponsePayload += fileNameSize;
				}

				context.response(ionet::PacketPayload(pResponse));
			});
		}
	}

	void RegisterStartDownloadFilesHandler(const std::weak_ptr<ReplicatorService>& pReplicatorServiceWeak, ionet::ServerPacketHandlers& handlers) {
//		RegisterFileDownloadHandler<ionet::PacketType::Start_Download_Files, ionet::PacketType::Start_Download_Files_Response>(
//			pReplicatorServiceWeak,
//			handlers,
//			[](auto pReplicatorService, const Key& consumer, const Key& driveKey, auto&& fileNames) {
//				return pReplicatorService->startDownloadFiles(consumer, driveKey, std::move(fileNames));
//			});
	}

	void RegisterStopDownloadFilesHandler(const std::weak_ptr<ReplicatorService>& pReplicatorServiceWeak, ionet::ServerPacketHandlers& handlers) {
//		RegisterFileDownloadHandler<ionet::PacketType::Stop_Download_Files, ionet::PacketType::Stop_Download_Files_Response>(
//			pReplicatorServiceWeak,
//			handlers,
//			[](auto pReplicatorService, const Key&, const Key& driveKey, auto&& fileNames) {
//				return pReplicatorService->stopDownloadFiles(driveKey, std::move(fileNames));
//			});
	}
}}