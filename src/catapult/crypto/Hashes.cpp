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

#include "Hashes.h"
#include "KeccakHash.h"
#include "catapult/utils/Casting.h"
#include "OpensslContexts.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif

extern "C" {
#include <ripemd160/ripemd160.h>
}
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <sha256/crypto_hash_sha256.h>

namespace catapult { namespace crypto {

	// region free functions

	void Ripemd160(const RawBuffer& dataBuffer, Hash160& hash) noexcept {
		struct ripemd160 context;
		ripemd160(&context, dataBuffer.pData, dataBuffer.Size);
		memcpy(hash.data(), context.u.u8, Hash160_Size);
	}

	namespace {
		void Sha256(crypto_hash_sha256_state& state, const RawBuffer& dataBuffer, Hash256& hash) {
			crypto_hash_sha256_init(&state);
			crypto_hash_sha256_update(&state, dataBuffer.pData, dataBuffer.Size);
			crypto_hash_sha256_final(&state, hash.data());
		}
		template<typename THash>
		void HashSingleBuffer(const EVP_MD* pMessageDigest, const RawBuffer& dataBuffer, THash& hash) {
			auto outputSize = static_cast<unsigned int>(hash.size());

			OpensslDigestContext context;
			context.dispatch(EVP_DigestInit_ex, pMessageDigest, nullptr);
			context.dispatch(EVP_DigestUpdate, dataBuffer.pData, dataBuffer.Size);
			context.dispatch(EVP_DigestFinal_ex, hash.data(), &outputSize);
		}
	}

	void Sha256(const RawBuffer& dataBuffer, Hash256& hash) {
		HashSingleBuffer(EVP_sha256(), dataBuffer, hash);
	}
	void Sha512(const RawBuffer& dataBuffer, Hash512& hash) {
		HashSingleBuffer(EVP_sha512(), dataBuffer, hash);
	}

	void Bitcoin160(const RawBuffer& dataBuffer, Hash160& hash) noexcept {
		crypto_hash_sha256_state state;
		Hash256 firstHash;
		Sha256(state, dataBuffer, firstHash);
		Ripemd160(firstHash, hash);
	}

	void Sha256Double(const RawBuffer& dataBuffer, Hash256& hash) noexcept {
		crypto_hash_sha256_state state;
		Hash256 firstHash;
		Sha256(state, dataBuffer, firstHash);
		Sha256(state, firstHash, hash);
	}

	namespace {
		template<typename TBuilder, typename THash>
		void HashSingleBuffer(const RawBuffer& dataBuffer, THash& hash) noexcept {
			TBuilder hashBuilder;
			hashBuilder.update(dataBuffer);
			hashBuilder.final(hash);
		}
	}

	void Sha3_256(const RawBuffer& dataBuffer, Hash256& hash) noexcept {
		HashSingleBuffer<Sha3_256_Builder>(dataBuffer, hash);
	}

	void Sha3_512(const RawBuffer& dataBuffer, Hash512& hash) noexcept {
		HashSingleBuffer<Sha3_512_Builder>(dataBuffer, hash);
	}

	void Keccak_256(const RawBuffer& dataBuffer, Hash256& hash) noexcept {
		HashSingleBuffer<Keccak_256_Builder>(dataBuffer, hash);
	}

	void Keccak_512(const RawBuffer& dataBuffer, Hash512& hash) noexcept {
		HashSingleBuffer<Keccak_512_Builder>(dataBuffer, hash);
	}

	void Hmac_Sha256(const RawBuffer& key, const RawBuffer& input, Hash256& output) {
		unsigned int outputSize = 0;
		HMAC(EVP_sha256(), key.pData, static_cast<int>(key.Size), input.pData, input.Size, output.data(), &outputSize);
	}
	
	void Hmac_Sha3_256(const RawBuffer& key, const RawBuffer& input, Hash256& output) {
		unsigned int outputSize = 0;
		HMAC(EVP_sha3_256(), key.pData, static_cast<int>(key.Size), input.pData, input.Size, output.data(), &outputSize);
	}

	void Hmac_Sha512(const RawBuffer& key, const RawBuffer& input, Hash512& output) {
		unsigned int outputSize = 0;
		HMAC(EVP_sha512(), key.pData, static_cast<int>(key.Size), input.pData, input.Size, output.data(), &outputSize);
	}
	// endregion

	// region sha3 / keccak builders

	namespace {

		Keccak_HashInstance* CastToKeccakHashInstance(uint8_t* pHashContext) noexcept {
			return reinterpret_cast<Keccak_HashInstance*>(pHashContext);
		}

		inline void KeccakInitialize(Keccak_HashInstance* pHashContext, Hash256_tag) {
			Keccak_HashInitialize_SHA3_256(pHashContext);
		}

		inline void KeccakInitialize(Keccak_HashInstance* pHashContext, Hash512_tag) {
			Keccak_HashInitialize_SHA3_512(pHashContext);
		}

		inline void KeccakInitialize(Keccak_HashInstance* pHashContext, GenerationHash_tag) {
			Keccak_HashInitialize_SHA3_256(pHashContext);
		}

		inline void KeccakFinal(uint8_t* context, uint8_t* output, int hashSize, KeccakModeTag) noexcept {
			Keccak_HashSqueeze(CastToKeccakHashInstance(context), output, static_cast<uint32_t>(hashSize * 8));
		}

		const EVP_MD* GetMessageDigest(Sha2ModeTag, Hash512_tag) {
			return EVP_sha512();
		}

		const EVP_MD* GetMessageDigest(Sha2ModeTag, Hash256_tag) {
			return EVP_sha256();
		}

		const EVP_MD* GetMessageDigest(Sha3ModeTag, Hash256_tag) {
			return EVP_sha3_256();
		}

		const EVP_MD* GetMessageDigest(Sha3ModeTag, Hash512_tag) {
			return EVP_sha3_512();
		}

		const EVP_MD* GetMessageDigest(Sha3ModeTag, GenerationHash_tag) {
			return EVP_sha3_256();
		}

	}

	//region hash builder
	template<typename TModeTag, typename THashTag>
	HashBuilderT<TModeTag, THashTag>::HashBuilderT() {
		m_context.dispatch(EVP_DigestInit_ex, GetMessageDigest(TModeTag(), THashTag()), nullptr);
	}

	template<typename TModeTag, typename THashTag>
	void HashBuilderT<TModeTag, THashTag>::update(const RawBuffer& dataBuffer) {
		m_context.dispatch(EVP_DigestUpdate, dataBuffer.pData, dataBuffer.Size);
	}

	template<typename TModeTag, typename THashTag>
	void HashBuilderT<TModeTag, THashTag>::update(std::initializer_list<const RawBuffer> buffers) {
		for (const auto& buffer : buffers)
			update(buffer);
	}

	template<typename TModeTag, typename THashTag>
	void HashBuilderT<TModeTag, THashTag>::final(OutputType& output) {
		auto outputSize = static_cast<unsigned int>(output.size());
		m_context.dispatch(EVP_DigestFinal_ex, output.data(), &outputSize);
	}

	//endregion

	//region keccak builder
	template<typename TModeTag, typename THashTag>
	KeccakBuilder<TModeTag, THashTag>::KeccakBuilder() {
		static_assert(sizeof(Keccak_HashInstance) <= sizeof(m_hashContext), "m_hashContext is too small to fit Keccak instance");
		KeccakInitialize(CastToKeccakHashInstance(m_hashContext), THashTag());
	}

	template<typename TModeTag, typename THashTag>
	void KeccakBuilder<TModeTag, THashTag>::update(const RawBuffer& dataBuffer) noexcept {
		Keccak_HashUpdate(CastToKeccakHashInstance(m_hashContext), dataBuffer.pData, dataBuffer.Size * 8);
	}

	template<typename TModeTag, typename THashTag>
	void KeccakBuilder<TModeTag, THashTag>::update(std::initializer_list<const RawBuffer> buffers) noexcept {
		for (const auto& buffer : buffers)
			update(buffer);
	}

	template<typename TModeTag, typename THashTag>
	void KeccakBuilder<TModeTag, THashTag>::final(OutputType& output) noexcept {
		KeccakFinal(m_hashContext, output.data(), THashTag::Size, TModeTag());
	}

	//endregion

	template class HashBuilderT<Sha3ModeTag, Hash256_tag>;
	template class HashBuilderT<Sha3ModeTag, Hash512_tag>;
	template class HashBuilderT<Sha2ModeTag, Hash512_tag>;
	template class HashBuilderT<Sha2ModeTag, Hash256_tag>;
	template class KeccakBuilder<KeccakModeTag, Hash256_tag>;
	template class KeccakBuilder<KeccakModeTag, Hash512_tag>;
	template class HashBuilderT<Sha3ModeTag, GenerationHash_tag>;

	// endregion
}}
