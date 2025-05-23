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

#pragma once
#include "src/model/LockHashAlgorithm.h"
#include "plugins/txes/lock_shared/src/model/LockNotifications.h"

namespace catapult { namespace model {

	// region lock secret notification types

/// Defines a lock secret notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_LOCKSECRET_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, LockSecret, DESCRIPTION, CODE)

	/// Lock secret duration.
	DEFINE_LOCKSECRET_NOTIFICATION(Secret_Duration_v1, 0x0001, Validator);

	/// Lock hash algorithm.
	DEFINE_LOCKSECRET_NOTIFICATION(Hash_Algorithm_v1, 0x0002, Validator);

	/// Lock secret.
	DEFINE_LOCKSECRET_NOTIFICATION(Secret_v1, 0x0003, All);

	/// Proof and secret.
	DEFINE_LOCKSECRET_NOTIFICATION(Proof_Secret_v1, 0x0004, Validator);

	/// Proof publication.
	DEFINE_LOCKSECRET_NOTIFICATION(Proof_Publication_v1, 0x0005, All);

#undef DEFINE_LOCKSECRET_NOTIFICATION

	// endregion

	/// Notification of a secret lock duration
	template<VersionType version>
	struct SecretLockDurationNotification;

	template<>
	struct SecretLockDurationNotification<1> : public BaseLockDurationNotification<SecretLockDurationNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockSecret_Secret_Duration_v1_Notification;

	public:
		using BaseLockDurationNotification<SecretLockDurationNotification<1>>::BaseLockDurationNotification;
	};

	/// Notification of a secret lock hash algorithm.
	template<VersionType version>
	struct SecretLockHashAlgorithmNotification;

	template<>
	struct SecretLockHashAlgorithmNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockSecret_Hash_Algorithm_v1_Notification;

	public:
		/// Creates secret lock hash algorithm notification around \a hashAlgorithm.
		SecretLockHashAlgorithmNotification(LockHashAlgorithm hashAlgorithm)
				: Notification(Notification_Type, sizeof(SecretLockHashAlgorithmNotification<1>))
				, HashAlgorithm(hashAlgorithm)
		{}

	public:
		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;
	};

	/// Notification of a secret lock.
	template<VersionType version>
	struct SecretLockNotification;

	template<>
	struct SecretLockNotification<1> : public BaseLockNotification<SecretLockNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockSecret_Secret_v1_Notification;

	public:
		/// Creates secret lock notification around \a signer, \a mosaic, \a duration, \a hashAlgorithm, \a secret and \a recipient.
		SecretLockNotification(
				const Key& signer,
				const UnresolvedMosaic& mosaic,
				BlockDuration duration,
				LockHashAlgorithm hashAlgorithm,
				const Hash256& secret,
				const UnresolvedAddress& recipient)
				: BaseLockNotification(signer, &mosaic, 1, duration)
				, HashAlgorithm(hashAlgorithm)
				, Secret(secret)
				, Recipient(recipient)
		{}

	public:
		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;

		/// Secret.
		Hash256 Secret;

		/// Recipient of the locked mosaic.
		UnresolvedAddress Recipient;
	};

	/// Notification of a secret and its proof.
	template<VersionType version>
	struct ProofSecretNotification;

	template<>
	struct ProofSecretNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockSecret_Proof_Secret_v1_Notification;

	public:
		/// Creates proof secret notification around \a hashAlgorithm, \a secret and \a proof.
		ProofSecretNotification(LockHashAlgorithm hashAlgorithm, const Hash256& secret, const RawBuffer& proof)
				: Notification(Notification_Type, sizeof(ProofSecretNotification<1>))
				, HashAlgorithm(hashAlgorithm)
				, Secret(secret)
				, Proof(proof)
		{}

	public:
		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;

		/// Secret.
		const Hash256& Secret;

		/// Proof.
		RawBuffer Proof;
	};

	/// Notification of a proof publication.
	template<VersionType version>
	struct ProofPublicationNotification;

	template<>
	struct ProofPublicationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockSecret_Proof_Publication_v1_Notification;

	public:
		/// Creates proof publication notification around \a signer, \a hashAlgorithm, \a secret and \a recipient.
		ProofPublicationNotification(
				const Key& signer,
				LockHashAlgorithm hashAlgorithm,
				const Hash256& secret,
				const UnresolvedAddress& recipient)
				: Notification(Notification_Type, sizeof(ProofPublicationNotification<1>))
				, Signer(signer)
				, HashAlgorithm(hashAlgorithm)
				, Secret(secret)
				, Recipient(recipient)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Hash algorithm.
		LockHashAlgorithm HashAlgorithm;

		/// Secret.
		Hash256 Secret;

		/// Recipient of the locked mosaic.
		UnresolvedAddress Recipient;
	};
}}
