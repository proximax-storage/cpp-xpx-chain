/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbViewFetcher.h"
#include "catapult/types.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cstdint>

namespace catapult { namespace dbrb {

	void Write(uint8_t*& pBuffer, const utils::RawBuffer& data);
	void Write(uint8_t*& pBuffer, uint8_t byte);
	void Write(uint8_t*& pBuffer, uint32_t value);
	struct View;
	void Write(uint8_t*& pBuffer, const View& view);
	void Write(uint8_t*& pBuffer, const Payload& payload);
	void Write(uint8_t*& pBuffer, const CertificateType& certificate);
	void Write(uint8_t*& pBuffer, const DbrbTreeView& view);

	template<typename T>
	T Read(const uint8_t*& pBuffer);
	template<>
	ProcessId Read(const uint8_t*& pBuffer);
	template<>
	Signature Read(const uint8_t*& pBuffer);
	template<>
	Hash256 Read(const uint8_t*& pBuffer);
	template<>
	View Read(const uint8_t*& pBuffer);
	template<>
	Payload Read(const uint8_t*& pBuffer);
	template<>
	CertificateType Read(const uint8_t*& pBuffer);
	template<>
	DbrbTreeView Read(const uint8_t*& pBuffer);

	Hash256 CalculateHash(const std::vector<RawBuffer>& buffers);
	Hash256 CalculatePayloadHash(const Payload& payload);

	std::string ProcessIdToString(const ProcessId& processId);
}}