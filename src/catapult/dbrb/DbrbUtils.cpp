/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbUtils.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace dbrb {

	void Write(uint8_t*& pBuffer, const utils::RawBuffer& data) {
		memcpy(pBuffer, data.pData, data.Size);
		pBuffer += data.Size;
	}

	void Write(uint8_t*& pBuffer, uint8_t byte) {
		memcpy(pBuffer, &byte, 1u);
		pBuffer++;
	}

	void Write(uint8_t*& pBuffer, const View& view) {
		auto size = utils::checked_cast<size_t, uint32_t>(view.Data.size());
		*reinterpret_cast<uint32_t*>(pBuffer) = size;
		pBuffer += sizeof(uint32_t);
		for (const auto& id : view.Data) {
			Write(pBuffer, id);
		}
	}

	void Write(uint8_t*& pBuffer, const Payload& payload) {
		memcpy(pBuffer, payload.get(), payload->Size);
		pBuffer += payload->Size;
	}

	void Write(uint8_t*& pBuffer, const CertificateType& certificate) {
		*reinterpret_cast<uint32_t*>(pBuffer) = utils::checked_cast<size_t, uint32_t>(certificate.size());
		pBuffer += sizeof(uint32_t);
		for (const auto& [id, signature] : certificate) {
			Write(pBuffer, id);
			Write(pBuffer, signature);
		}
	}

	template<>
	ProcessId Read(const uint8_t*& pBuffer) {
		ProcessId id;
		std::memcpy(id.data(), pBuffer, ProcessId_Size);
		pBuffer += ProcessId_Size;

		return id;
	}

	template<>
	Signature Read(const uint8_t*& pBuffer) {
		Signature signature;
		std::memcpy(signature.data(), pBuffer, Signature_Size);
		pBuffer += Signature_Size;

		return signature;
	}

	template<>
	Hash256 Read(const uint8_t*& pBuffer) {
		Hash256 hash;
		std::memcpy(hash.data(), pBuffer, Hash256_Size);
		pBuffer += Hash256_Size;

		return hash;
	}

	template<>
	View Read(const uint8_t*& pBuffer) {
		View view;
		auto count = *reinterpret_cast<const uint32_t*>(pBuffer);
		pBuffer += sizeof(uint32_t);
		for (auto i = 0u; i < count; ++i) {
			auto id = Read<ProcessId>(pBuffer);
			view.Data.emplace(id);
		}

		return view;
	}

	template<>
	Payload Read(const uint8_t*& pBuffer) {
		auto packetSize = *reinterpret_cast<const uint32_t*>(pBuffer);
		auto payload = ionet::CreateSharedPacket<ionet::Packet>(packetSize - sizeof(ionet::Packet));
		memcpy(payload.get(), pBuffer, packetSize);
		pBuffer += packetSize;

		return payload;
	}

	template<>
	CertificateType Read(const uint8_t*& pBuffer) {
		CertificateType certificate;
		auto count = *reinterpret_cast<const uint32_t*>(pBuffer);
		pBuffer += sizeof(uint32_t);
		for (auto i = 0u; i < count; ++i) {
			auto id = Read<ProcessId>(pBuffer);
			auto signature = Read<Signature>(pBuffer);
			certificate[id] = signature;
		}

		return certificate;
	}

	Hash256 CalculateHash(const std::vector<RawBuffer>& buffers) {
		crypto::Sha3_256_Builder hashBuilder;
		for (const auto& buffer : buffers)
			hashBuilder.update(buffer);

		Hash256 hash;
		hashBuilder.final(hash);
		return hash;
	}

	Hash256 CalculatePayloadHash(const Payload& payload) {
		return CalculateHash({{ reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }});
	}

	std::string ProcessIdToString(const ProcessId& processId) {

		std::ostringstream processIdStringStream;
		processIdStringStream << processId;
		const std::string processIdString = processIdStringStream.str();

		std::ostringstream out;
		out << processIdString.substr(0, 3);

		return out.str();
	}

	bool View::isMember(const ProcessId& processId) const {
		return Data.find(processId) != Data.end();
	}

	size_t View::quorumSize() const {
		const size_t size = Data.size();
		return size - (size - 1) / 3;
	}

	size_t View::packedSize() const {
		return sizeof(uint32_t) + Data.size() * ProcessId_Size;
	}

	bool View::operator==(const View& other) const {
		return Data == other.Data;
	}

	bool View::operator!=(const View& other) const {
		return Data != other.Data;
	}

	bool View::operator<(const View& other) const {
		const auto& otherData = other.Data;
		const bool notEqual = (*this != other);
		return notEqual && std::includes(otherData.begin(), otherData.end(), Data.begin(), Data.end());
	}

	bool View::operator>(const View& other) const {
		const auto& otherData = other.Data;
		const bool notEqual = (*this != other);
		return notEqual && std::includes(Data.begin(), Data.end(), otherData.begin(), otherData.end());
	}

	bool View::operator<=(const View& other) const {
		return (*this < other) || (*this == other);
	}

	bool View::operator>=(const View& other) const {
		return (*this > other) || (*this == other);
	}

	View& View::merge(const View& other) {
		Data.insert(other.Data.begin(), other.Data.end());
		return *this;
	}

	View& View::difference(const View& other) {
		const auto& otherData = other.Data;
		ViewData newData;
		std::set_difference(Data.begin(), Data.end(), otherData.begin(), otherData.end(), std::inserter(newData, newData.begin()));
		Data = std::move(newData);
		return *this;
	}
}}