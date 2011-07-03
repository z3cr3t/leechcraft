/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Alexander Konovalov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "cryptosystem.h"
#include "ciphertextformat.h"
#include <QByteArray>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

using namespace LeechCraft::Plugins::SecMan::StoragePlugins::SecureStorage;

CryptoSystem::CryptoSystem (const QString &password)
{
	Key_ = CreateKey (password);
}

CryptoSystem::~CryptoSystem ()
{
	Key_.fill (0, Key_.length ());
}

QByteArray CryptoSystem::Encrypt (const QByteArray &data) const
{
	QByteArray result;
	result.resize (CipherTextFormat::BufferLengthFor (data.length ()));
	CipherTextFormat cipherText (result.data (), data.length ());

	// fill IV in cipherText & random block
	// TODO: check error codes.
	RAND_bytes (cipherText.Iv (), IV_LENGTH);
	unsigned char randomData[RND_LENGTH];
	RAND_bytes (randomData, RND_LENGTH);

	// init cipher
	EVP_CIPHER_CTX cipherCtx;
	EVP_CIPHER_CTX_init (&cipherCtx);
	EVP_EncryptInit (&cipherCtx, EVP_aes_256_ofb (), reinterpret_cast<const unsigned char*> (Key_.data ()), cipherText.Iv ());

	// encrypt
	int outLength = 0;
	unsigned char* outPtr = cipherText.Data ();
	EVP_EncryptUpdate (&cipherCtx, outPtr, &outLength, reinterpret_cast<const unsigned char*> (data.data ()), data.length ());
	outPtr += outLength;
	EVP_EncryptUpdate (&cipherCtx, outPtr, &outLength, randomData, RND_LENGTH);
	outPtr += outLength;
	// output last block & cleanup
	EVP_EncryptFinal (&cipherCtx, outPtr, &outLength);
	outPtr += outLength;

	// compute hmac
	HMAC_CTX hmacCtx;
	HMAC_CTX_init (&hmacCtx);
	HMAC_Init_ex (&hmacCtx, Key_.data (), Key_.length (), EVP_sha256 (), 0);
	HMAC_Update (&hmacCtx, reinterpret_cast<const unsigned char*> (data.data ()), data.length ());
	HMAC_Update (&hmacCtx, randomData, RND_LENGTH);
	unsigned hmacLength = 0;
	HMAC_Final (&hmacCtx, cipherText.Hmac (), &hmacLength);
	HMAC_CTX_cleanup (&hmacCtx);

	return result;
}

QByteArray CryptoSystem::Decrypt (const QByteArray &cipherText) const
{
	if (CipherTextFormat::DataLengthFor (cipherText.length ()) < 0)
		throw WrongHMACException ();
	QByteArray data;
	data.resize (CipherTextFormat::DecryptBufferLengthFor (cipherText.length ()));
	CipherTextFormat cipherTextFormat (const_cast<char*> (cipherText.data ()),
			CipherTextFormat::DataLengthFor (cipherText.length ()));
	// init cipher
	EVP_CIPHER_CTX cipherCtx;
	EVP_CIPHER_CTX_init (&cipherCtx);
	EVP_DecryptInit (&cipherCtx, EVP_aes_256_ofb (),
			reinterpret_cast<const unsigned char*> (Key_.data ()), cipherTextFormat.Iv ());
	// decrypt
	int outLength = 0;
	unsigned char* outPtr = reinterpret_cast<unsigned char*> (data.data ());
	EVP_DecryptUpdate (&cipherCtx, outPtr, &outLength,
			cipherTextFormat.Data (), cipherTextFormat.DataLength_);
	outPtr += outLength;
	EVP_DecryptUpdate (&cipherCtx, outPtr, &outLength,
			cipherTextFormat.Rnd (), RND_LENGTH);
	outPtr += outLength;
	// output last block & cleanup
	EVP_DecryptFinal (&cipherCtx, outPtr, &outLength);
	outPtr += outLength;

	// compute hmac
	unsigned char hmac[HMAC_LENGTH];
	HMAC_CTX hmacCtx;
	HMAC_CTX_init (&hmacCtx);
	HMAC_Init_ex (&hmacCtx, Key_.data (), Key_.length (),
			EVP_sha256 (), 0);
	HMAC_Update (&hmacCtx, reinterpret_cast<unsigned char*> (data.data ()), data.length ());
	unsigned int hmacLength = 0;
	HMAC_Final (&hmacCtx, hmac, &hmacLength);
	HMAC_CTX_cleanup (&hmacCtx);

	// check hmac
	bool hmacsDifferent = memcmp (hmac, cipherTextFormat.Hmac (), HMAC_LENGTH);
	if (hmacsDifferent)
		throw WrongHMACException ();
	// remove random block
	data.truncate (cipherTextFormat.DataLength_);
	return data;
}

QByteArray CryptoSystem::Hash (const QByteArray &data) const
{
	unsigned char hash[HASH_LENGTH];
	SHA256 (reinterpret_cast<const unsigned char*> (data.data ()), data.size (), hash);
	return QByteArray (reinterpret_cast<char*> (hash), HASH_LENGTH);
}

QByteArray CryptoSystem::CreateKey (const QString &password) const
{
	QByteArray res = Hash (password.toUtf8 ());
	res.resize (KEY_LENGTH);
	return res;
}

