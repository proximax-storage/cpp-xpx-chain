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
		for (const auto& [id, change] : view.Data) {
			Write(pBuffer, id);
			Write(pBuffer, utils::to_underlying_type(change));
		}
	}

	void Write(uint8_t*& pBuffer, const Sequence& sequence) {
		auto size = utils::checked_cast<size_t, uint32_t>(sequence.data().size());
		*reinterpret_cast<uint32_t*>(pBuffer) = size;
		pBuffer += sizeof(uint32_t);
		for (const auto& view : sequence.data())
			Write(pBuffer, view);
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
	MembershipChange Read(const uint8_t*& pBuffer) {
		return static_cast<MembershipChange>(*pBuffer++);
	}

	template<>
	View Read(const uint8_t*& pBuffer) {
		View view;
		auto count = *reinterpret_cast<const uint32_t*>(pBuffer);
		pBuffer += sizeof(uint32_t);
		for (auto i = 0u; i < count; ++i) {
			auto id = Read<ProcessId>(pBuffer);
			auto change = Read<MembershipChange>(pBuffer);
			view.Data.emplace(id, change);
		}

		return view;
	}

	template<>
	Sequence Read(const uint8_t*& pBuffer) {
		Sequence sequence;
		auto count = *reinterpret_cast<const uint32_t*>(pBuffer);
		pBuffer += sizeof(uint32_t);
		for (auto i = 0u; i < count; ++i) {
			auto view = Read<View>(pBuffer);
			sequence.tryAppend(view);
		}

		return sequence;
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

	std::string MembershipChangeToString(const ProcessId& processId, const MembershipChange& change) {
		std::ostringstream out;
		out << (change == MembershipChange::Join ? "+" : "-");

		std::ostringstream processIdStringStream;
		processIdStringStream << processId;
		const std::string processIdString = processIdStringStream.str();

		//const auto firstDigitPos = processIdStringStream.str().find_first_of("0123456789");
		out << processIdString.substr(0, 3);

		return out.str();
	}

	std::set<ProcessId> View::members() const {
		std::set<ProcessId> joined;
		std::set<ProcessId> left;
		std::set<ProcessId> current;

		for (const auto& [node, changeType] : Data) {
			if (changeType == MembershipChange::Join)
				joined.insert(node);
			else // MembershipChange::Leave
				left.insert(node);
		}

		std::set_difference(joined.begin(), joined.end(),
							left.begin(), left.end(),
							std::inserter(current, current.begin()));

		return current;
	}

	bool View::hasChange(const ProcessId& processId, MembershipChange change) const {
		return std::find_if(Data.cbegin(), Data.cend(), [&processId, change](const auto& pair) { return pair.first == processId && pair.second == change; }) != Data.cend();
	}

	bool View::isMember(const ProcessId& processId) const {
		return hasChange(processId, MembershipChange::Join) && !hasChange(processId, MembershipChange::Leave);
	}

	size_t View::quorumSize() const {
		const size_t size = members().size();
		return size - (size - 1) / 3;
	}

	size_t View::packedSize() const {
		return sizeof(uint32_t) + Data.size() * (ProcessId_Size + 1u);
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

	const std::vector<View>& Sequence::data() const {
		return m_data;
	}

	size_t Sequence::packedSize() const {
		size_t packedSize = sizeof(uint32_t);
		for (const auto& view : m_data)
			packedSize += view.packedSize();
		return packedSize;
	}

	std::optional<View> Sequence::maybeLeastRecent() const {
		if (!m_data.empty())
			return m_data.front();
		return {};
	}

	std::optional<View> Sequence::maybeMostRecent() const {
		if (!m_data.empty())
			return m_data.back();
		return {};
	}

	int64_t Sequence::canInsert(const View& testedView) const {
		const auto pMostRecent = maybeMostRecent();
		if (!pMostRecent)
			return 0;	// Sequence is empty; any view can be inserted and will appear at index 0.

		// Tested view is comparable with every view in Data IFF it is comparable
		// with the biggest view in Data, i.e. the most recent one.
		if (!View::areComparable(testedView, *pMostRecent))
			return SIZE_MAX;

		// Since at this point we know that testedView is comparable with every view in Data,
		// we can now use comparison by size for faster processing.
		const auto testedViewSize = testedView.Data.size();
		auto pView = m_data.begin();
		int64_t pos = 0;
		for (; pView != m_data.end(); ++pos, ++pView) {
			if (testedViewSize == pView->Data.size())
				return SIZE_MAX;	// Duplicate means non-strictly ascending order.
			if (testedViewSize < pView->Data.size())
				return pos;
		}

		// testedView is the most recent view and is appended to the end of Data.
		return pos;
	}

	bool Sequence::canAppend(const View& testedView) const {
		const auto pMostRecent = maybeMostRecent();
		if (!pMostRecent)
			return true;	// Sequence is empty; any view can be appended.

		// Tested view can be appended to Data IFF it is strictly greater than the most recent view from Data.
		return testedView > *pMostRecent;
	}

	bool Sequence::canAppend(const Sequence& testedSequence) const {
		const auto pThisMostRecent = maybeMostRecent();
		const auto pOtherLeastRecent = testedSequence.maybeLeastRecent();
		if (!pThisMostRecent || !pOtherLeastRecent)
			return true;	// If either sequence is empty, append is always legal.

		return *pThisMostRecent < *pOtherLeastRecent;
	}

	bool Sequence::canMerge(const Sequence& testedSequence) const {
		const auto& sequenceData = testedSequence.data();
		bool mergeable;
		for (const auto& view : sequenceData) {
			mergeable = (canInsert(view) != SIZE_MAX)
						|| (std::find(sequenceData.begin(), sequenceData.end(), view) != sequenceData.end());
			if (!mergeable)
				break;
		}
		return mergeable;
	}

	std::vector<View>::iterator Sequence::tryInsert(const View& newView) {
		const int64_t pos = canInsert(newView);
		if (pos != SIZE_MAX)
			return m_data.insert(m_data.begin() + pos, newView);
		else
			return m_data.end();
	}

	bool Sequence::tryAppend(const View& newView) {
		const bool appendable = canAppend(newView);
		if (appendable)
			m_data.push_back(newView);

		return appendable;
	}

	bool Sequence::tryAppend(const Sequence& newSequence) {
		const bool appendable = canAppend(newSequence);
		if (appendable)
			m_data.insert(m_data.end(), newSequence.data().begin(), newSequence.data().end());

		return appendable;
	}

	bool Sequence::tryMerge(const Sequence& newSequence) {
		const bool mergeable = canMerge(newSequence);
		if (mergeable) {
			for (const auto& view : newSequence.data()) {
				tryInsert(view);
			}
		}
		return mergeable;
	}

	bool Sequence::tryErase(const View& view) {
		const auto iter = std::find(m_data.begin(), m_data.end(), view);
		const bool found = (iter != m_data.end());
		if (found)
			m_data.erase(iter);

		return found;
	}

	bool Sequence::operator==(const Sequence& rhs) const {
		return m_data == rhs.m_data;
	}

	bool Sequence::operator<(const Sequence& rhs) const {
		return m_data.size() < rhs.m_data.size();
	}

	Sequence::Sequence(const std::vector<View>& sequenceData) {
		m_data = sequenceData;
	}
}}