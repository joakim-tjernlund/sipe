/**
 * @file tests.c
 *
 * pidgin-sipe
 *
 * Copyright (C) 2010 pier11 <pier11@operamail.com>
 * Copyright (C) 2008 Novell, Inc.
 *
 * Implemented with reference to the follow documentation:
 *   - http://davenport.sourceforge.net/ntlm.html
 *   - MS-NLMP: http://msdn.microsoft.com/en-us/library/cc207842.aspx
 *   - MS-SIP : http://msdn.microsoft.com/en-us/library/cc246115.aspx
 *
 * Build and run with (adjust as needed to your build platform!)
 *
 * $ gcc -I /usr/include/libpurple \
 *       -I /usr/include/dbus-1.0 -I /usr/lib/dbus-1.0/include \
 *       -I /usr/include/glib-2.0 -I /usr/lib/glib-2.0/include \
 *       -o tests tests.c sipe-sign.c sipmsg.c sip-sec.c uuid.c -lpurple
 * ./tests
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <glib.h>
#include <stdlib.h>

#include "sipe-sign.h"
#define _SIPE_COMPILING_TESTS
#include "sip-sec-ntlm.c"

#include "uuid.h"

static int successes = 0;
static int failures = 0;

static size_t
hex_str_to_buff(const char *hex_str, guint8 **bytes)
{
	guint8 *buff;
	char two_digits[3];
	size_t i;

	if (!bytes) return 0;

	gsize length = strlen(hex_str)/2;
	buff = (guint8 *)g_malloc(length);
	for (i = 0; i < length; i++) {
		two_digits[0] = hex_str[i * 2];
		two_digits[1] = hex_str[i * 2 + 1];
		two_digits[2] = '\0';
		buff[i] = (guint8)strtoul(two_digits, NULL, 16);
	}

	*bytes = buff;
	return length;
}

static void assert_equal(const char * expected, const guchar * got, int len, gboolean stringify)
{
	const gchar * res = (gchar *) got;
	gchar to_str[len*2];

	if (stringify) {
		int i, j;
		for (i = 0, j = 0; i < len; i++, j+=2) {
			g_sprintf(&to_str[j], "%02X", (got[i]&0xff));
		}
		len *= 2;
		res = to_str;
	}

	printf("expected: %s\n", expected);
	printf("received: %s\n", res);

	if (g_ascii_strncasecmp(expected, res, len) == 0) {
		successes++;
		printf("PASSED\n");
	} else {
		failures++;
		printf("FAILED\n");
	}
}

/* NOTE: both values are expected to be in host byte order! */
static void assert_equal_guint32(guint32 expected, guint32 got)
{
	printf("expected: %08X\n", expected);
	printf("received: %08X\n", got);

	if (expected == got) {
		successes++;
		printf("PASSED\n");
	} else {
		failures++;
		printf("FAILED\n");
	}
}

int main()
{
	printf ("Starting Tests\n");

	/* Initialization that libpurple/core.c would normally do */
	purple_debug_init();
	purple_debug_set_enabled(TRUE);
	purple_ciphers_init();

	/* These tests are from the MS-SIPE document */

	const char * password = "Password";
	const char * user = "User";
	const char * domain = "Domain";
	const guchar client_challenge [] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
	/* server challenge */
	const guchar nonce [] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
	/* 16 bytes */
	const guchar exported_session_key[] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
	const guchar text [] = {0x50, 0x00, 0x6c, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00}; //P·l·a·i·n·t·e·x·t·


////// internal Cyphers tests ///////
	printf ("\nTesting MD4()\n");
	guchar md4 [16];
	MD4 ((const unsigned char *)"message digest", 14, md4);
	assert_equal("D9130A8164549FE818874806E1C7014B", md4, 16, TRUE);

	printf ("\nTesting MD5()\n");
	guchar md5 [16];
	MD5 ((const unsigned char *)"message digest", 14, md5);
	assert_equal("F96B697D7CB7938D525A2F31AAF161D0", md5, 16, TRUE);

	printf ("\nTesting HMAC_MD5()\n");
	guchar hmac_md5 [16];
	HMAC_MD5 ((const unsigned char *)"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 16, (const unsigned char *)"Hi There", 8, hmac_md5);
	assert_equal("9294727A3638BB1C13F48EF8158BFC9D", hmac_md5, 16, TRUE);


////// NTLMv1 (without Extended Session Security) ///////
	use_ntlm_v2 = FALSE;

	guint32 flags = 0
		| NTLMSSP_NEGOTIATE_KEY_EXCH
		| NTLMSSP_NEGOTIATE_56
		| NTLMSSP_NEGOTIATE_128
		| NTLMSSP_NEGOTIATE_VERSION
		| NTLMSSP_TARGET_TYPE_SERVER
		| NTLMSSP_NEGOTIATE_ALWAYS_SIGN
		| NTLMSSP_NEGOTIATE_NTLM
		| NTLMSSP_NEGOTIATE_SEAL
		| NTLMSSP_NEGOTIATE_SIGN
		| NTLMSSP_NEGOTIATE_OEM
		| NTLMSSP_NEGOTIATE_UNICODE;

	printf ("\n\nTesting Negotiation Flags\n");
	assert_equal_guint32(0xE2028233, flags);

	printf ("\n\nTesting LMOWFv1()\n");
	guchar response_key_lm [16];
	LMOWFv1 (password, user, domain, response_key_lm);
	assert_equal("E52CAC67419A9A224A3B108F3FA6CB6D", response_key_lm, 16, TRUE);

	printf ("\n\nTesting NTOWFv1()\n");
	guchar response_key_nt [16];
	NTOWFv1 (password, user, domain, response_key_nt);
	assert_equal("A4F49C406510BDCAB6824EE7C30FD852", response_key_nt, 16, TRUE);

	printf ("\n\nTesting LM Response Generation\n");
	printf ("Testing NT Response Generation\n");
	printf ("Testing Session Base Key\n");
	guchar nt_challenge_response [24];
	guchar lm_challenge_response [24];
	guchar session_base_key [16];

	compute_response(flags,
			 response_key_nt,
			 response_key_lm,
			 nonce,
			 client_challenge,
			 0,
			 NULL, /* target_info */
			 0,  /* target_info_len */
			 lm_challenge_response,	/* out */
			 nt_challenge_response,	/* out */
			 session_base_key);	/* out */

	assert_equal("98DEF7B87F88AA5DAFE2DF779688A172DEF11C7D5CCDEF13", lm_challenge_response, 24, TRUE);
	assert_equal("67C43011F30298A2AD35ECE64F16331C44BDBED927841F94", nt_challenge_response, 24, TRUE);
	assert_equal("D87262B0CDE4B1CB7499BECCCDF10784", session_base_key, 16, TRUE);

	printf ("\n\nTesting Key Exchange Key\n");
	guchar key_exchange_key [16];
	KXKEY(flags, session_base_key, lm_challenge_response, nonce, key_exchange_key);
	assert_equal("D87262B0CDE4B1CB7499BECCCDF10784", key_exchange_key, 16, TRUE);

	printf ("\n\nTesting Encrypted Session Key Generation\n");
	guchar encrypted_random_session_key [16];
	RC4K (key_exchange_key, 16, exported_session_key, 16, encrypted_random_session_key);
	assert_equal("518822B1B3F350C8958682ECBB3E3CB7", encrypted_random_session_key, 16, TRUE);

	printf ("\n\nTesting CRC32\n");
	guint32 crc = CRC32((char*)text, 18);
	assert_equal_guint32(0x93AA847D, crc);

	printf ("\n\nTesting Encryption\n");
	guchar client_seal_key [16];
	//SEALKEY (flags, exported_session_key, TRUE, client_seal_key);
	guchar buff [18 + 12];
	memcpy(buff, text, 18);
	guchar text_enc [18 + 12];
	guint32 to_enc [3];
	to_enc[0] = GUINT32_TO_LE(0); // random pad
	to_enc[1] = GUINT32_TO_LE(crc);
	to_enc[2] = GUINT32_TO_LE(0); // zero
	memcpy(buff+18, (gchar *)to_enc, 12);
	RC4K (exported_session_key, 16, buff, 18 + 12, text_enc);
	//The point is to not reinitialize rc4 cypher
	//                                                   0          crc        0 (zero)
	assert_equal("56FE04D861F9319AF0D7238A2E3B4D457FB8" "45C844E5" "09DCD1DF" "2E459D36", text_enc, 18 + 12, TRUE);

	printf ("\n\nTesting MAC\n");
	// won't work in the case with sealing because RC4 is re-initialized inside.
	//gchar *mac = MAC (flags, (gchar*)text, 18, (guchar*)exported_session_key, 16, (guchar*)exported_session_key,16,  0x00000000,  0);
	guint32 enc [3];
	memcpy((gchar *)enc, text_enc+18, 12);
	guint32 mac2 [4];
	mac2 [0] = GUINT32_TO_LE(1); // version
	mac2 [1] = enc [0];
	mac2 [2] = enc [1];
	mac2 [3] = enc [2] ^ (GUINT32_TO_LE(0)); // ^ seq
	assert_equal("0100000045C844E509DCD1DF2E459D36", (guchar*)mac2, 16, TRUE);


////// EXTENDED_SESSIONSECURITY ///////
	use_ntlm_v2 = FALSE;
	flags = 0
		| NTLMSSP_NEGOTIATE_56
		| NTLMSSP_NEGOTIATE_VERSION
		| NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
		| NTLMSSP_TARGET_TYPE_SERVER
		| NTLMSSP_NEGOTIATE_ALWAYS_SIGN
		| NTLMSSP_NEGOTIATE_NTLM
		| NTLMSSP_NEGOTIATE_SEAL
		| NTLMSSP_NEGOTIATE_SIGN
		| NTLMSSP_NEGOTIATE_OEM
		| NTLMSSP_NEGOTIATE_UNICODE;

	printf ("\n\n(Extended session security) Testing Negotiation Flags\n");
	assert_equal_guint32(0x820A8233, flags);

	/* NTOWFv1() is not different from the above test for the same */

	printf ("\n\n(Extended session security) Testing LM Response\n");
	printf ("(Extended session security) Testing NT Response\n");
	printf ("(Extended session security) Testing Session Base Key\n");
	compute_response(flags,
			 response_key_nt,
			 response_key_lm,
			 nonce,
			 client_challenge,
			 0,
			 NULL, /* target_info */
			 0,  /* target_info_len */
			 lm_challenge_response,	/* out */
			 nt_challenge_response,	/* out */
			 session_base_key);	/* out */

	assert_equal("AAAAAAAAAAAAAAAA00000000000000000000000000000000", lm_challenge_response, 24, TRUE);
	assert_equal("7537F803AE367128CA458204BDE7CAF81E97ED2683267232", nt_challenge_response, 24, TRUE);
	assert_equal("D87262B0CDE4B1CB7499BECCCDF10784", session_base_key, 16, TRUE);

	printf ("\n\n(Extended session security) Testing Key Exchange Key\n");
	KXKEY(flags, session_base_key, lm_challenge_response, nonce, key_exchange_key);
	assert_equal("EB93429A8BD952F8B89C55B87F475EDC", key_exchange_key, 16, TRUE);

	printf ("\n\n(Extended session security) SIGNKEY\n");
	guchar client_sign_key [16];
	SIGNKEY (key_exchange_key, TRUE, client_sign_key);
	assert_equal("60E799BE5C72FC92922AE8EBE961FB8D", client_sign_key, 16, TRUE);

	printf ("\n\n(Extended session security) SEALKEY\n");
	SEALKEY (flags, key_exchange_key, TRUE, client_seal_key);
	assert_equal("04DD7F014D8504D265A25CC86A3A7C06", client_seal_key, 16, TRUE);

	printf ("\n\n(Extended session security) Testing Encryption\n");
	RC4K (client_seal_key, 16, text, 18, text_enc);
	assert_equal("A02372F6530273F3AA1EB90190CE5200C99D", text_enc, 18, TRUE);

	printf ("\n\n(Extended session security) Testing MAC\n");
	gchar *mac = MAC (flags,   (gchar*)text,18,   client_sign_key,16,   client_seal_key,16,   0,  0);
	assert_equal("01000000FF2AEB52F681793A00000000", (guchar*)mac, 32, FALSE);
	g_free(mac);


////// NTLMv2 ///////
	use_ntlm_v2 = TRUE;
	flags = 0
		| NTLMSSP_NEGOTIATE_KEY_EXCH
		| NTLMSSP_NEGOTIATE_56
		| NTLMSSP_NEGOTIATE_128
		| NTLMSSP_NEGOTIATE_VERSION
		| NTLMSSP_NEGOTIATE_TARGET_INFO
		| NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
		| NTLMSSP_TARGET_TYPE_SERVER
		| NTLMSSP_NEGOTIATE_ALWAYS_SIGN
		| NTLMSSP_NEGOTIATE_NTLM
		| NTLMSSP_NEGOTIATE_SEAL
		| NTLMSSP_NEGOTIATE_SIGN
		| NTLMSSP_NEGOTIATE_OEM
		| NTLMSSP_NEGOTIATE_UNICODE;

	printf ("\n\nTesting (NTLMv2) Negotiation Flags\n");
	assert_equal_guint32(0xE28A8233, flags);

	printf ("\n\nTesting NTOWFv2()\n");
	NTOWFv2 (password, user, domain, response_key_nt);
	NTOWFv2 (password, user, domain, response_key_lm);
	assert_equal("0C868A403BFD7A93A3001EF22EF02E3F", response_key_nt, 16, TRUE);


	printf ("\n\nTesting (NTLMv2) LM Response Generation\n");
	printf ("Testing (NTLMv2) NT Response Generation and Session Base Key\n");
/*
Challenge:
4e544c4d53535000020000000c000c003800000033828ae20123456789abcdef00000000000000002400240044000000060070170000000f53006500720076006500720002000c0044006f006d00610069006e0001000c0053006500720076006500720000000000

        NTLMSSP_NEGOTIATE_UNICODE
        NTLMSSP_NEGOTIATE_OEM
        NTLMSSP_NEGOTIATE_SIGN
        NTLMSSP_NEGOTIATE_SEAL
        NTLMSSP_NEGOTIATE_NTLM
        NTLMSSP_NEGOTIATE_ALWAYS_SIGN
        NTLMSSP_TARGET_TYPE_SERVER
        NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
        NTLMSSP_NEGOTIATE_TARGET_INFO
        NTLMSSP_NEGOTIATE_VERSION
        NTLMSSP_NEGOTIATE_128
        NTLMSSP_NEGOTIATE_KEY_EXCH
        NTLMSSP_NEGOTIATE_56
        target_name.len   : 12
        target_name.maxlen: 12
        target_name.offset: 56
        target_info.len   : 36
        target_info.maxlen: 36
        target_info.offset: 68
        product: 6.0.6000 (Windows Vista, Windows Server 2008, Windows 7 or Windows Server 2008 R2)
        ntlm_revision_current: 0x0F (NTLMSSP_REVISION_W2K3)
        target_name: Server
        MsvAvNbDomainName: Domain
        MsvAvNbComputerName: Server

target_name:
530065007200760065007200
target_info:
02000c0044006f006d00610069006e0001000c0053006500720076006500720000000000

Response:
4e544c4d5353500003000000180018006c00000054005400840000000c000c00480000000800080054000000100010005c00000010001000d8000000358288e20501280a0000000f44006f006d00610069006e00550073006500720043004f004d005000550054004500520086c35097ac9cec102554764a57cccc19aaaaaaaaaaaaaaaa68cd0ab851e51c96aabc927bebef6a1c01010000000000000000000000000000aaaaaaaaaaaaaaaa0000000002000c0044006f006d00610069006e0001000c005300650072007600650072000000000000000000c5dad2544fc9799094ce1ce90bc9d03e


*/
	const guint64 time_val = 0;
	const guint8 target_info [] = {
		0x02, 0x00, 0x0C, 0x00, //NetBIOS Domain name, 4 bytes
		0x44, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6E, 0x00, //D.o.m.a.i.n.  12bytes
		0x01, 0x00, 0x0C, 0x00, //NetBIOS Server name, 4 bytes
		0x53, 0x00, 0x65, 0x00, 0x72, 0x00, 0x76, 0x00, 0x65, 0x00, 0x72, 0x00, //S.e.r.v.e.r.  12bytes
		0x00, 0x00, 0x00, 0x00, //Av End, 4 bytes
		};
	const int target_info_len = 32+4;
	int ntlmssp_nt_resp_len = (16 + (32+target_info_len));
	guchar nt_challenge_response_v2 [ntlmssp_nt_resp_len];

	compute_response(flags,
			 response_key_nt,
			 response_key_lm,
			 nonce,
			 client_challenge,
			 time_val,
			 target_info, /* target_info */
			 target_info_len,  /* target_info_len */
			 lm_challenge_response,	/* out */
			 nt_challenge_response_v2,	/* out */
			 session_base_key);	/* out */

	assert_equal("86C35097AC9CEC102554764A57CCCC19AAAAAAAAAAAAAAAA", lm_challenge_response, 24, TRUE);
	assert_equal("68CD0AB851E51C96AABC927BEBEF6A1C", nt_challenge_response_v2, 16, TRUE);
	/* the ref string is taken from binary dump of AUTHENTICATE_MESSAGE */
	assert_equal("68CD0AB851E51C96AABC927BEBEF6A1C01010000000000000000000000000000AAAAAAAAAAAAAAAA0000000002000C0044006F006D00610069006E0001000C005300650072007600650072000000000000000000", nt_challenge_response_v2, ntlmssp_nt_resp_len, TRUE);
	assert_equal("8DE40CCADBC14A82F15CB0AD0DE95CA3", session_base_key, 16, TRUE);

	printf ("\n\nTesting (NTLMv2) Encrypted Session Key\n");
	// key_exchange_key = session_base_key for NTLMv2
	KXKEY(flags, session_base_key, lm_challenge_response, nonce, key_exchange_key);
	//RC4 encryption of the RandomSessionKey with the KeyExchangeKey:
	RC4K (key_exchange_key, 16, exported_session_key, 16, encrypted_random_session_key);
	assert_equal("C5DAD2544FC9799094CE1CE90BC9D03E", encrypted_random_session_key, 16, TRUE);

	printf ("\n\nTesting (NTLMv2) SIGNKEY\n");
	SIGNKEY (exported_session_key, TRUE, client_sign_key);
	assert_equal("4788DC861B4782F35D43FD98FE1A2D39", client_sign_key, 16, TRUE);

	printf ("\n\nTesting (NTLMv2) SEALKEY\n");
	SEALKEY (flags, exported_session_key, TRUE, client_seal_key);
	assert_equal("59F600973CC4960A25480A7C196E4C58", client_seal_key, 16, TRUE);

	printf ("\n\nTesting (NTLMv2) Encryption\n");
	RC4K (client_seal_key, 16, text, 18, text_enc);
	assert_equal("54E50165BF1936DC996020C1811B0F06FB5F", text_enc, 18, TRUE);

//	printf ("\n\nTesting (NTLMv2) Encryption\n");
//const guchar text2 [] = {0x50, 0x00, 0x6c, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00
//			, 0x70, 0x35, 0x28, 0x51, 0xf2, 0x56, 0x43, 0x09}; //P·l·a·i·n·t·e·x·t·
//guchar text_enc2 [18+8];
//	RC4K (client_seal_key, 16, text2, 18+8, text_enc2);
//	assert_equal("54E50165BF1936DC996020C1811B0F06FB5F", text_enc2, 18+8, TRUE);

	printf ("\n\nTesting (NTLMv2) MAC (without RC4, as we don't keep its handle yet)\n");
	mac = MAC (flags & ~NTLMSSP_NEGOTIATE_KEY_EXCH,   (gchar*)text,18,   client_sign_key,16,   client_seal_key,16,   0,  0);
	assert_equal("0100000070352851F256430900000000", (guchar*)mac, 32, FALSE);
	g_free(mac);


	/* End tests from the MS-SIPE document */


////// davenport tests ///////
	// Test from http://davenport.sourceforge.net/ntlm.html#ntlm1Signing
	const gchar *text_j = "jCIFS";
	printf ("\n\n(davenport) Testing Signature Algorithm\n");
	guchar sk [] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xe5, 0x38, 0xb0};
	assert_equal (
		"0100000078010900397420FE0E5A0F89",
		(guchar *) MAC(NEGOTIATE_FLAGS & ~NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY, text_j, strlen(text_j), sk, 8,  sk,8,  0x00090178, 0),
		32, FALSE
	);

	// Tests from http://davenport.sourceforge.net/ntlm.html#ntlm2Signing
	printf ("\n\n(davenport) SIGNKEY\n");
	const guchar master_key [] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00};
	SIGNKEY (master_key, TRUE, client_sign_key);
	assert_equal("F7F97A82EC390F9C903DAC4F6ACEB132", client_sign_key, 16, TRUE);

	printf ("\n\n(davenport) Testing MAC - no Key Exchange flag\n");
	mac = MAC (flags & ~NTLMSSP_NEGOTIATE_KEY_EXCH, text_j, strlen(text_j), client_sign_key, 16,  client_sign_key,16,  0,  0);
	assert_equal("010000000A003602317A759A00000000", (guchar*)mac, 32, FALSE);
	g_free(mac);


////// SIPE internal tests ///////
	// Verify signature of SIPE message received from OCS 2007 after authenticating with pidgin-sipe
	printf ("\n\nTesting MS-SIPE Example Message Signing\n");
	char * msg1 = "<NTLM><0878F41B><1><SIP Communications Service><ocs1.ocs.provo.novell.com><8592g5DCBa1694i5887m0D0Bt2247b3F38xAE9Fx><3><REGISTER><sip:gabriel@ocs.provo.novell.com><2947328781><B816D65C2300A32CFA6D371F2AF537FD><900><200>";
	guchar exported_session_key2 [] = { 0x5F, 0x02, 0x91, 0x53, 0xBC, 0x02, 0x50, 0x58, 0x96, 0x95, 0x48, 0x61, 0x5E, 0x70, 0x99, 0xBA };
	assert_equal (
		"0100000000000000BF2E52667DDF6DED",
		(guchar *) MAC(NEGOTIATE_FLAGS & ~NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY, msg1, strlen(msg1), exported_session_key2, 16,  exported_session_key2,16,  0, 100),
		32, FALSE
	);

	// Verify parsing of message and signature verification
	printf ("\n\nTesting MS-SIPE Example Message Parsing, Signing, and Verification\n(Authentication Protocol Version 2)\n");
	char * msg2 = "SIP/2.0 200 OK\r\nms-keep-alive: UAS; tcp=no; hop-hop=yes; end-end=no; timeout=300\r\nAuthentication-Info: NTLM rspauth=\"0100000000000000BF2E52667DDF6DED\", srand=\"0878F41B\", snum=\"1\", opaque=\"4452DFB0\", qop=\"auth\", targetname=\"ocs1.ocs.provo.novell.com\", realm=\"SIP Communications Service\"\r\nFrom: \"Gabriel Burt\"<sip:gabriel@ocs.provo.novell.com>;tag=2947328781;epid=1234567890\r\nTo: <sip:gabriel@ocs.provo.novell.com>;tag=B816D65C2300A32CFA6D371F2AF537FD\r\nCall-ID: 8592g5DCBa1694i5887m0D0Bt2247b3F38xAE9Fx\r\nCSeq: 3 REGISTER\r\nVia: SIP/2.0/TLS 164.99.194.49:10409;branch=z9hG4bKE0E37DBAF252C3255BAD;received=164.99.195.20;ms-received-port=10409;ms-received-cid=1E00\r\nContact: <sip:164.99.195.20:10409;transport=tls;ms-received-cid=1E00>;expires=900\r\nExpires: 900\r\nAllow-Events: vnd-microsoft-provisioning,vnd-microsoft-roaming-contacts,vnd-microsoft-roaming-ACL,presence,presence.wpending,vnd-microsoft-roaming-self,vnd-microsoft-provisioning-v2\r\nSupported: adhoclist\r\nServer: RTC/3.0\r\nSupported: com.microsoft.msrtc.presence\r\nContent-Length: 0\r\n\r\n";
	struct sipmsg * msg = sipmsg_parse_msg(msg2);
	struct sipmsg_breakdown msgbd;
	msgbd.msg = msg;
	sipmsg_breakdown_parse(&msgbd, "SIP Communications Service", "ocs1.ocs.provo.novell.com");
	gchar * msg_str = sipmsg_breakdown_get_string(2, &msgbd);
	gchar * sig = sip_sec_ntlm_sipe_signature_make (NEGOTIATE_FLAGS & ~NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY, msg_str, 0, exported_session_key2, exported_session_key2);
	sipmsg_breakdown_free(&msgbd);
	assert_equal ("0100000000000000BF2E52667DDF6DED", (guchar *) sig, 32, FALSE);
	printf("purple_ntlm_verify_signature result = %i\n", sip_sec_ntlm_verify_signature (sig, "0100000000000000BF2E52667DDF6DED"));


////// real Communicator 2007 R2 tests //////
////// Recreated/verifyed real authentication communication between
////// Communicator 2007 R2 and Office Communications Server 2007 R2
////// with SIPE NTLMv2 implementation.

	const char *password2 = "Pa$$word";
	const char *user2 = "User";
	const char *domain2 = "COSMO";

//Challenge:
//TlRMTVNTUAACAAAAAAAAADgAAADzgpji3Ruq9OfiGNEAAAAAAAAAAJYAlgA4AAAABQLODgAAAA8CAAoAQwBPAFMATQBPAAEAGABDAE8AUwBNAE8ALQBPAEMAUwAtAFIAMgAEABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAMAMABjAG8AcwBtAG8ALQBvAGMAcwAtAHIAMgAuAGMAbwBzAG0AbwAuAGwAbwBjAGEAbAAFABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAAAAAA=
/*
Message (length 206):
        NTLMSSP_NEGOTIATE_UNICODE
        NTLMSSP_NEGOTIATE_OEM
        NTLMSSP_NEGOTIATE_SIGN
        NTLMSSP_NEGOTIATE_SEAL
        NTLMSSP_NEGOTIATE_DATAGRAM
        NTLMSSP_NEGOTIATE_LM_KEY
        NTLMSSP_NEGOTIATE_NTLM
        NTLMSSP_NEGOTIATE_ALWAYS_SIGN
        NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
        NTLMSSP_NEGOTIATE_IDENTIFY
        NTLMSSP_NEGOTIATE_TARGET_INFO
        NTLMSSP_NEGOTIATE_VERSION
        NTLMSSP_NEGOTIATE_128
        NTLMSSP_NEGOTIATE_KEY_EXCH
        NTLMSSP_NEGOTIATE_56
        server_challenge: DD1BAAF4E7E218D1
        target_name.len   : 0
        target_name.maxlen: 0
        target_name.offset: 56
        target_info.len   : 150
        target_info.maxlen: 150
        target_info.offset: 56
        product: 5.2.3790 (Windows Server 2003)
        ntlm_revision_current: 0x0F (NTLMSSP_REVISION_W2K3)
        target_info raw: 02000A0043004F0053004D004F000100180043004F0053004D004F002D004F00430053002D00520032000400160063006F0073006D006F002E006C006F00630061006C000300300063006F0073006D006F002D006F00630073002D00720032002E0063006F0073006D006F002E006C006F00630061006C000500160063006F0073006D006F002E006C006F00630061006C0000000000
        MsvAvNbDomainName: COSMO
        MsvAvNbComputerName: COSMO-OCS-R2
        MsvAvDnsDomainName: cosmo.local
        MsvAvDnsComputerName: cosmo-ocs-r2.cosmo.local
        MsvAvDnsTreeName: cosmo.local
*/


//Response:
//TlRMTVNTUAADAAAAGAAYAHIAAADGAMYAigAAAAoACgBIAAAACAAIAFIAAAAYABgAWgAAABAAEABQAQAAVYKYYgUCzg4AAAAPQwBPAFMATQBPAFUAcwBlAHIAQwBPAFMATQBPAC0ATwBDAFMALQBSADIAoeku/k4Hi/fFwASazGFmwtauh1yw/apBjcDIAK527KYG0rn769BHMQEBAAAAAAAAWVGaFye5ygHWrodcsP2qQQAAAAACAAoAQwBPAFMATQBPAAEAGABDAE8AUwBNAE8ALQBPAEMAUwAtAFIAMgAEABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAMAMABjAG8AcwBtAG8ALQBvAGMAcwAtAHIAMgAuAGMAbwBzAG0AbwAuAGwAbwBjAGEAbAAFABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAAAAAAAAAAAMctznhyoCkmFkeiueXEV5A==
/*
Message (length 352):
        NTLMSSP_NEGOTIATE_UNICODE
        NTLMSSP_REQUEST_TARGET
        NTLMSSP_NEGOTIATE_SIGN
        NTLMSSP_NEGOTIATE_DATAGRAM
        NTLMSSP_NEGOTIATE_NTLM
        NTLMSSP_NEGOTIATE_ALWAYS_SIGN
        NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
        NTLMSSP_NEGOTIATE_IDENTIFY
        NTLMSSP_NEGOTIATE_TARGET_INFO
        NTLMSSP_NEGOTIATE_VERSION
        NTLMSSP_NEGOTIATE_128
        NTLMSSP_NEGOTIATE_KEY_EXCH
        lm_resp.len   : 24
        lm_resp.maxlen: 24
        lm_resp.offset: 114
        nt_resp.len   : 198
        nt_resp.maxlen: 198
        nt_resp.offset: 138
        domain.len   : 10
        domain.maxlen: 10
        domain.offset: 72
        user.len   : 8
        user.maxlen: 8
        user.offset: 82
        host.len   : 24
        host.maxlen: 24
        host.offset: 90
        session_key.len   : 16
        session_key.maxlen: 16
        session_key.offset: 336
        product: 5.2.3790 (Windows Server 2003)
        ntlm_revision_current: 0x0F (NTLMSSP_REVISION_W2K3)
        lm_resp: A1E92EFE4E078BF7C5C0049ACC6166C2D6AE875CB0FDAA41
        nt_resp raw: 8DC0C800AE76ECA606D2B9FBEBD04731010100000000000059519A1727B9CA01D6AE875CB0FDAA410000000002000A0043004F0053004D004F000100180043004F0053004D004F002D004F00430053002D00520032000400160063006F0073006D006F002E006C006F00630061006C000300300063006F0073006D006F002D006F00630073002D00720032002E0063006F0073006D006F002E006C006F00630061006C000500160063006F0073006D006F002E006C006F00630061006C000000000000000000
        nt_resp: 8DC0C800AE76ECA606D2B9FBEBD04731
        target_info raw: 02000A0043004F0053004D004F000100180043004F0053004D004F002D004F00430053002D00520032000400160063006F0073006D006F002E006C006F00630061006C000300300063006F0073006D006F002D006F00630073002D00720032002E0063006F0073006D006F002E006C006F00630061006C000500160063006F0073006D006F002E006C006F00630061006C0000000000
        response_version: 1
        hi_response_version: 1
        time: 59519A1727B9CA01 - Mon Mar 01 10:08:08 2010
        client_challenge: D6AE875CB0FDAA41
        MsvAvNbDomainName: COSMO
        MsvAvNbComputerName: COSMO-OCS-R2
        MsvAvDnsDomainName: cosmo.local
        MsvAvDnsComputerName: cosmo-ocs-r2.cosmo.local
        MsvAvDnsTreeName: cosmo.local
        ----------- end of nt_resp v2 -----------
        domain: COSMO
        user: User
        host: COSMO-OCS-R2
        session_key: 31CB739E1CA80A498591E8AE797115E4
*/

	const char *request =
	"REGISTER sip:cosmo.local SIP/2.0\r\n"
	"Via: SIP/2.0/TLS 192.168.172.6:12723\r\n"
	"Max-Forwards: 70\r\n"
	"From: <sip:user@cosmo.local>;tag=3e49177a52;epid=c8ca638a15\r\n"
	"To: <sip:user@cosmo.local>\r\n"
	"Call-ID: 4037df9284354df39065195bd57a4b14\r\n"
	"CSeq: 3 REGISTER\r\n"
	"Contact: <sip:192.168.172.6:12723;transport=tls;ms-opaque=fad3dfab32>;methods=\"INVITE, MESSAGE, INFO, OPTIONS, BYE, CANCEL, NOTIFY, ACK, REFER, BENOTIFY\";proxy=replace;+sip.instance=\"<urn:uuid:34D859DB-6585-5F91-A3B4-DE853C15347D>\"\r\n"
	"User-Agent: UCCAPI/3.5.6907.0 OC/3.5.6907.0 (Microsoft Office Communicator 2007 R2)\r\n"
	"Supported: gruu-10, adhoclist, msrtc-event-categories\r\n"
	"Supported: ms-forking\r\n"
	"ms-keep-alive: UAC;hop-hop=yes\r\n"
	"Event: registration\r\n"
	"Proxy-Authorization: NTLM qop=\"auth\", realm=\"SIP Communications Service\", opaque=\"2BDBAC9D\", targetname=\"cosmo-ocs-r2.cosmo.local\", version=4, gssapi-data=\"TlRMTVNTUAADAAAAGAAYAHIAAADGAMYAigAAAAoACgBIAAAACAAIAFIAAAAYABgAWgAAABAAEABQAQAAVYKYYgUCzg4AAAAPQwBPAFMATQBPAFUAcwBlAHIAQwBPAFMATQBPAC0ATwBDAFMALQBSADIAoeku/k4Hi/fFwASazGFmwtauh1yw/apBjcDIAK527KYG0rn769BHMQEBAAAAAAAAWVGaFye5ygHWrodcsP2qQQAAAAACAAoAQwBPAFMATQBPAAEAGABDAE8AUwBNAE8ALQBPAEMAUwAtAFIAMgAEABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAMAMABjAG8AcwBtAG8ALQBvAGMAcwAtAHIAMgAuAGMAbwBzAG0AbwAuAGwAbwBjAGEAbAAFABYAYwBvAHMAbQBvAC4AbABvAGMAYQBsAAAAAAAAAAAAMctznhyoCkmFkeiueXEV5A==\", crand=\"13317733\", cnum=\"1\", response=\"0100000029618e9651b65a7764000000\"\r\n"
	"Content-Length: 0\r\n"
	"\r\n";

	const gchar *request_sig = "<NTLM><13317733><1><SIP Communications Service><cosmo-ocs-r2.cosmo.local><4037df9284354df39065195bd57a4b14><3><REGISTER><sip:user@cosmo.local><3e49177a52><sip:user@cosmo.local><><><><>";
//Signature:
//0100000029618e9651b65a7764000000

	const char *response =
	"SIP/2.0 200 OK\r\n"
	"ms-keep-alive: UAS; tcp=no; hop-hop=yes; end-end=no; timeout=300\r\n"
	"Authentication-Info: NTLM rspauth=\"01000000E615438A917661BE64000000\", srand=\"9616454F\", snum=\"1\", opaque=\"2BDBAC9D\", qop=\"auth\", targetname=\"cosmo-ocs-r2.cosmo.local\", realm=\"SIP Communications Service\"\r\n"
	"From: \"User\"<sip:user@cosmo.local>;tag=3e49177a52;epid=c8ca638a15\r\n"
	"To: <sip:user@cosmo.local>;tag=5E61CCD925D17E043D9A74835A88F664\r\n"
	"Call-ID: 4037df9284354df39065195bd57a4b14\r\n"
	"CSeq: 3 REGISTER\r\n"
	"Via: SIP/2.0/TLS 192.168.172.6:12723;ms-received-port=12723;ms-received-cid=2600\r\n"
	"Contact: <sip:192.168.172.6:12723;transport=tls;ms-opaque=fad3dfab32;ms-received-cid=2600>;expires=7200;+sip.instance=\"<urn:uuid:34d859db-6585-5f91-a3b4-de853c15347d>\";gruu=\"sip:user@cosmo.local;opaque=user:epid:21nYNIVlkV-jtN6FPBU0fQAA;gruu\"\r\n"
	"Expires: 7200\r\n"
	"presence-state: register-action=\"added\"\r\n"
	"Allow-Events: vnd-microsoft-provisioning,vnd-microsoft-roaming-contacts,vnd-microsoft-roaming-ACL,presence,presence.wpending,vnd-microsoft-roaming-self,vnd-microsoft-provisioning-v2\r\n"
	"Supported: adhoclist\r\n"
	"Server: RTC/3.5\r\n"
	"Supported: msrtc-event-categories\r\n"
	"Content-Length: 0\r\n"
	"\r\n";

	const gchar *response_sig = "<NTLM><9616454F><1><SIP Communications Service><cosmo-ocs-r2.cosmo.local><4037df9284354df39065195bd57a4b14><3><REGISTER><sip:user@cosmo.local><3e49177a52><sip:user@cosmo.local><5E61CCD925D17E043D9A74835A88F664><><><7200><200>";
//Signature:
//01000000E615438A917661BE64000000

	use_ntlm_v2 = TRUE;
	flags = 0
		| NTLMSSP_NEGOTIATE_UNICODE
		| NTLMSSP_REQUEST_TARGET
		| NTLMSSP_NEGOTIATE_SIGN
		| NTLMSSP_NEGOTIATE_DATAGRAM
		| NTLMSSP_NEGOTIATE_NTLM
		| NTLMSSP_NEGOTIATE_ALWAYS_SIGN
		| NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
		| NTLMSSP_NEGOTIATE_IDENTIFY
		| NTLMSSP_NEGOTIATE_TARGET_INFO
		| NTLMSSP_NEGOTIATE_VERSION
		| NTLMSSP_NEGOTIATE_128
		| NTLMSSP_NEGOTIATE_KEY_EXCH;

	NTOWFv2 (password2, user2, domain2, response_key_nt);
	NTOWFv2 (password2, user2, domain2, response_key_lm);

	guint8 *buff2;
	hex_str_to_buff("59519A1727B9CA01", &buff2);
	const guint64 time_val2 = GUINT64_FROM_LE(*((guint64 *)buff2));
	g_free(buff2);

	guint8 *target_info2;
	const int target_info2_len = hex_str_to_buff("02000A0043004F0053004D004F000100180043004F0053004D004F002D004F00430053002D00520032000400160063006F0073006D006F002E006C006F00630061006C000300300063006F0073006D006F002D006F00630073002D00720032002E0063006F0073006D006F002E006C006F00630061006C000500160063006F0073006D006F002E006C006F00630061006C0000000000", &target_info2);

	guint8 *nonce2;
	hex_str_to_buff("DD1BAAF4E7E218D1", &nonce2);

	guint8 *client_challenge2;
	hex_str_to_buff("D6AE875CB0FDAA41", &client_challenge2);

	ntlmssp_nt_resp_len = (16 + (32+target_info2_len));
	guchar nt_challenge_response_v2_2 [ntlmssp_nt_resp_len];

	printf ("\n\nTesting (NTLMv2 / OC 2007 R2) LM Response Generation\n");
	printf (    "Testing (NTLMv2 / OC 2007 R2) NT Response Generation\n");
	compute_response(flags,
			 response_key_nt,
			 response_key_lm,
			 nonce2,
			 client_challenge2,
			 time_val2,
			 target_info2, /* target_info */
			 target_info2_len,  /* target_info_len */
			 lm_challenge_response,	/* out */
			 nt_challenge_response_v2_2,	/* out */
			 session_base_key);	/* out */
	g_free(client_challenge2);
	g_free(nonce2);
	g_free(target_info2);

	assert_equal("A1E92EFE4E078BF7C5C0049ACC6166C2D6AE875CB0FDAA41", lm_challenge_response, 24, TRUE);
	assert_equal("8DC0C800AE76ECA606D2B9FBEBD04731", nt_challenge_response_v2_2, 16, TRUE);
	/* the ref string is taken from binary dump of AUTHENTICATE_MESSAGE */
	assert_equal("8DC0C800AE76ECA606D2B9FBEBD04731010100000000000059519A1727B9CA01D6AE875CB0FDAA410000000002000A0043004F0053004D004F000100180043004F0053004D004F002D004F00430053002D00520032000400160063006F0073006D006F002E006C006F00630061006C000300300063006F0073006D006F002D006F00630073002D00720032002E0063006F0073006D006F002E006C006F00630061006C000500160063006F0073006D006F002E006C006F00630061006C000000000000000000", nt_challenge_response_v2_2, ntlmssp_nt_resp_len, TRUE);

	KXKEY(flags, session_base_key, lm_challenge_response, nonce2, key_exchange_key);
	//as in the Type3 message
	guint8 *encrypted_random_session_key2;
	hex_str_to_buff("31CB739E1CA80A498591E8AE797115E4", &encrypted_random_session_key2);
	guchar exported_session_key3[16];
	//decoding exported_session_key
	RC4K (key_exchange_key, 16, encrypted_random_session_key2, 16, exported_session_key3);
	g_free(encrypted_random_session_key2);

	guchar server_sign_key [16];
	guchar server_seal_key [16];
	SIGNKEY (exported_session_key3, TRUE, client_sign_key);
	SEALKEY (flags, exported_session_key3, TRUE, client_seal_key);
	SIGNKEY (exported_session_key3, FALSE, server_sign_key);
	SEALKEY (flags, exported_session_key3, FALSE, server_seal_key);

	printf ("\n\nTesting (NTLMv2 / OC 2007 R2) Message Parsing, Signing, and Verification\nClient request\n(Authentication Protocol version 4)\n");
	msg = sipmsg_parse_msg(request);
	msgbd.msg = msg;
	sipmsg_breakdown_parse(&msgbd, "SIP Communications Service", "cosmo-ocs-r2.cosmo.local");
	msg_str = sipmsg_breakdown_get_string(4, &msgbd);
	assert_equal (request_sig, (guchar *)msg_str, strlen(request_sig), FALSE);
	sig = sip_sec_ntlm_sipe_signature_make (flags, msg_str, 0, client_sign_key, client_seal_key);
	sipmsg_breakdown_free(&msgbd);
	assert_equal ("0100000029618e9651b65a7764000000", (guchar *) sig,32, FALSE);
	printf("purple_ntlm_verify_signature result = %i\n", sip_sec_ntlm_verify_signature (sig, "0100000029618e9651b65a7764000000"));

	printf ("\n\nTesting (NTLMv2 / OC 2007 R2) Message Parsing, Signing, and Verification\nServer response\n(Authentication Protocol version 4)\n");
	msg = sipmsg_parse_msg(response);
	msgbd.msg = msg;
	sipmsg_breakdown_parse(&msgbd, "SIP Communications Service", "cosmo-ocs-r2.cosmo.local");
	msg_str = sipmsg_breakdown_get_string(4, &msgbd);
	assert_equal (response_sig, (guchar *)msg_str, strlen(response_sig), FALSE);
	// server keys here
	sig = sip_sec_ntlm_sipe_signature_make (flags, msg_str, 0, server_sign_key, server_seal_key);
	sipmsg_breakdown_free(&msgbd);
	assert_equal ("01000000E615438A917661BE64000000", (guchar *) sig,32, FALSE);
	printf("purple_ntlm_verify_signature result = %i\n", sip_sec_ntlm_verify_signature (sig, "01000000E615438A917661BE64000000"));

	printf ("\n\nTesting (NTLMv2 / OC 2007 R2) MAC - client signing\n");
	mac = MAC (flags,   (gchar*)request_sig,strlen(request_sig),   client_sign_key,16,   client_seal_key,16,   0,  100);
	assert_equal("0100000029618e9651b65a7764000000", (guchar*)mac, 32, FALSE);
	g_free(mac);

	printf ("\n\nTesting (NTLMv2 / OC 2007 R2) MAC - server's verifying\n");
	mac = MAC (flags,   (gchar*)response_sig,strlen(response_sig),   server_sign_key,16,   server_seal_key,16,   0,  100);
	assert_equal("01000000E615438A917661BE64000000", (guchar*)mac, 32, FALSE);
	g_free(mac);


////// UUID tests ///////
	/* begin tests from MS-SIPRE */

	const char *testEpid = "01010101";
	const char *expectedUUID = "4b1682a8-f968-5701-83fc-7c6741dc6697";
	gchar *calcUUID = generateUUIDfromEPID(testEpid);

	printf("\n\nTesting MS-SIPRE UUID derivation\n");

	assert_equal(expectedUUID, (guchar *) calcUUID, strlen(expectedUUID), FALSE);
	g_free(calcUUID);

	guchar addr[6];
	gchar nmac[6];

	int i,j;
	for (i = 0,j=0; i < 6; i++,j+=2) {
		g_sprintf(&nmac[j], "%02X", addr[i]);
	}

	printf("Mac: %s\n", g_strdup(nmac));

	/* end tests from MS-SIPRE */

	printf ("\nFinished With Tests; %d successs %d failures\n", successes, failures);

	return(0);
}

/*
  Local Variables:
  mode: c
  c-file-style: "bsd"
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
