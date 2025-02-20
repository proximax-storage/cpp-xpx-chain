/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "CertificateUtils.h"
#include "catapult/exceptions.h"
#include <cstring>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <openssl/bio.h>
#include <openssl/dh.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace catapult { namespace crypto {

	namespace {
		// region MemoryStream

		class MemoryStream {
		public:
			MemoryStream() : m_pImpl(std::shared_ptr<BIO>(BIO_new(BIO_s_mem()), BIO_free)) {
				if (!m_pImpl)
					throw std::bad_alloc();
			}

		public:
			bool printEx(const X509_NAME& name) {
				return -1 != X509_NAME_print_ex(m_pImpl.get(), &name, 0, XN_FLAG_RFC2253);
			}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

			std::string toString() {
				char* pRawData;
				auto dataSize = BIO_get_mem_data(m_pImpl.get(), &pRawData);

				std::string result;
				if (pRawData) {
					result.resize(static_cast<size_t>(dataSize));
					std::memcpy(result.data(), pRawData, result.size());
				}

				return result;
			}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

		private:
			std::shared_ptr<BIO> m_pImpl;
		};

		// endregion

		bool ExtractOneLine(const X509_NAME& name, std::string& result) {
			MemoryStream memoryStream;
			if (!memoryStream.printEx(name))
				return false;

			result = memoryStream.toString();
			return true;
		}
	}

	bool TryParseCertificate(const X509& certificate, CertificateInfo& certificateInfo) {
		const auto* pSubject = X509_get_subject_name(&certificate);
		if (!ExtractOneLine(*pSubject, certificateInfo.Subject))
			return false;

		auto nullTerminatorIndex = certificateInfo.Subject.find_last_not_of('\0');
		certificateInfo.Subject.resize(std::string::npos == nullTerminatorIndex ? 0 : nullTerminatorIndex + 1);

		const auto* pCertificatePublicKey = X509_get0_pubkey(&certificate);
		if (!pCertificatePublicKey)
			return false;

		if (EVP_PKEY_ED25519 != EVP_PKEY_id(pCertificatePublicKey))
			return false;

		auto keySize = certificateInfo.PublicKey.size();
		return EVP_PKEY_get_raw_public_key(pCertificatePublicKey, certificateInfo.PublicKey.data(), &keySize) && Key_Size == keySize;
	}

	namespace {
		template<typename TObject, typename TFunc, typename... TArgs>
		bool Dispatch(const std::shared_ptr<TObject>& pObject, TFunc func, TArgs&&... args) {
			if (!pObject)
				throw std::bad_alloc();

			return func(pObject.get(), std::forward<TArgs>(args)...);
		}
	}

	bool VerifySelfSigned(X509& certificate) {
		auto pCertificateStore = std::shared_ptr<X509_STORE>(X509_STORE_new(), X509_STORE_free);
		if (!Dispatch(pCertificateStore, X509_STORE_add_cert, &certificate))
			return false;

		auto pCertificateStoreContext = std::shared_ptr<X509_STORE_CTX>(X509_STORE_CTX_new(), X509_STORE_CTX_free);
		if (!Dispatch(pCertificateStoreContext, X509_STORE_CTX_init, pCertificateStore.get(), &certificate, nullptr))
			return false;

		X509_STORE_CTX_set_flags(pCertificateStoreContext.get(), X509_V_FLAG_CHECK_SS_SIGNATURE);
		return 1 == X509_verify_cert(pCertificateStoreContext.get());
	}

	namespace {
		std::shared_ptr<EVP_PKEY> GenerateCertificateKey(const crypto::KeyPair& keyPair) {
			auto i = 0u;
			Key rawPrivateKey;
			for (auto byte : keyPair.privateKey())
				rawPrivateKey[i++] = byte;

			return std::shared_ptr<EVP_PKEY>(
					EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, rawPrivateKey.data(), rawPrivateKey.size()),
					EVP_PKEY_free);
		}

		std::shared_ptr<EVP_PKEY> GenerateCertificatePublicKey(const Key& publicKey) {
			return std::shared_ptr<EVP_PKEY>(
					EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, publicKey.data(), publicKey.size()),
					EVP_PKEY_free);
		}

		void AddTextEntry(X509_NAME& name, const std::string& key, const std::string& value) {
			if (!X509_NAME_add_entry_by_txt(&name, key.data(), MBSTRING_ASC, reinterpret_cast<const uint8_t*>(value.data()), -1, -1, 0))
				CATAPULT_THROW_RUNTIME_ERROR_1("failed to add text entry", key);
		}

		bool DeleteEntry(X509_NAME& name) {
			auto* pEntry = X509_NAME_delete_entry(&name, 0);
			if (!pEntry)
				return false;

			X509_NAME_ENTRY_free(pEntry);
			return true;
		}

		void SetTextEntries(X509_NAME& name, const std::string& country, const std::string& organization, const std::string& commonName) {
			while (DeleteEntry(name)) {}
			AddTextEntry(name, "C", country);
			AddTextEntry(name, "O", organization);
			AddTextEntry(name, "CN", commonName);
		}

		struct BioWrapper {
		public:
			BioWrapper() : m_pBio(std::shared_ptr<BIO>(BIO_new(BIO_s_mem()), BIO_free)) {
				if (!m_pBio)
					throw std::bad_alloc();
			}

		public:
			operator BIO*() const {
				return m_pBio.get();
			}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

			std::string toString() const {
				char* pBioData = nullptr;
				auto bioSize = static_cast<size_t>(BIO_get_mem_data(m_pBio.get(), &pBioData));
				return std::string(pBioData, pBioData + bioSize);
			}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

		private:
			std::shared_ptr<BIO> m_pBio;
		};
	}

	std::string CreatePrivateKeyPem(const crypto::KeyPair& keyPair) {
		BioWrapper bio;
		auto pKey = GenerateCertificateKey(keyPair);
		if (!PEM_write_bio_PrivateKey(bio, pKey.get(), nullptr, nullptr, 0, nullptr, nullptr))
			CATAPULT_THROW_RUNTIME_ERROR("error writing private key to bio");

		return bio.toString();
	}

	std::string CreatePublicKeyPem(const crypto::KeyPair& keyPair) {
		BioWrapper bio;
		auto pKey = GenerateCertificateKey(keyPair);
		if (!PEM_write_bio_PUBKEY(bio, pKey.get()))
			CATAPULT_THROW_RUNTIME_ERROR("error writing public key to bio");

		return bio.toString();
	}

	std::string CreateFullCertificateChainPem(const std::vector<std::shared_ptr<X509>>& certificates) {
		BioWrapper bio;

		for (const auto& pCertificate : certificates) {
			if (!PEM_write_bio_X509(bio, pCertificate.get()))
				CATAPULT_THROW_RUNTIME_ERROR("error writing certificate to bio");
		}

		return bio.toString();
	}

	CertificateBuilder::CertificateBuilder() : m_pCertificate(std::shared_ptr<X509>(X509_new(), X509_free)) {
		if (!m_pCertificate)
			throw std::bad_alloc();

		// set the version
		if (!X509_set_version(get(), 0))
			CATAPULT_THROW_RUNTIME_ERROR("failed to set certificate version");

		// set the serial number
		if (!ASN1_INTEGER_set(X509_get_serialNumber(get()), 1))
			CATAPULT_THROW_RUNTIME_ERROR("failed to set certificate serial number");

		// set expiration from now until one year from now

		setValidity(0, 365 * 24 * 60 * 60);
	}

	void CertificateBuilder::setSubject(const std::string& country, const std::string& organization, const std::string& commonName) {
		SetTextEntries(*X509_get_subject_name(get()), country, organization, commonName);
	}

	void CertificateBuilder::setIssuer(const std::string& country, const std::string& organization, const std::string& commonName) {
		SetTextEntries(*X509_get_issuer_name(get()), country, organization, commonName);
	}

	void CertificateBuilder::setPublicKey(const Key& publicKey) {
		auto pKey = GenerateCertificatePublicKey(publicKey);
		if (!X509_set_pubkey(get(), pKey.get()))
			CATAPULT_THROW_RUNTIME_ERROR("failed to set certificate public key");
	}

	void CertificateBuilder::setPublicKey(const crypto::KeyPair& keyPair) {
		auto pKey = GenerateCertificateKey(keyPair);
		Key publicKey;
		auto publicKeyBufferSize = Key_Size;
		if (!EVP_PKEY_get_raw_public_key(pKey.get(), publicKey.data(), &publicKeyBufferSize))
			CATAPULT_THROW_RUNTIME_ERROR("failed to get public key");

		setPublicKey(publicKey);
	}

	void CertificateBuilder::setValidity(long startDate, long endDate) {
		if (!X509_gmtime_adj(X509_getm_notBefore(get()), startDate) || !X509_gmtime_adj(X509_getm_notAfter(get()), endDate))
			CATAPULT_THROW_RUNTIME_ERROR("failed to set certificate expiration");
	}

	std::shared_ptr<X509> CertificateBuilder::build() {
		return m_pCertificate;
	}

	std::shared_ptr<X509> CertificateBuilder::buildAndSign(const crypto::KeyPair& keyPair) {
		auto pKeyPair = GenerateCertificateKey(keyPair);
		if (0 == X509_sign(get(), pKeyPair.get(), EVP_md_null()))
			CATAPULT_THROW_RUNTIME_ERROR("failed to sign certificate");

		return build();
	}

	X509* CertificateBuilder::get() {
		return m_pCertificate.get();
	}
}}
