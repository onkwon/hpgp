/*
 * SPDX-FileCopyrightText: 2023 Kyunghwan Kwon <k@mononn.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef HPGP_H
#define HPGP_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "hpgp_mme.h"

#define HPGP_MMTYPE_MSB_BIT		13

typedef enum {
	HPGP_RC_UNKNOWN,
	HPGP_RC_READY,
	HPGP_RC_IN_PROGRESS,
	HPGP_RC_MATCHED,
	HPGP_RC_INVALID_INPUT,
	HPGP_RC_INCORRECT_INPUT,
} hpgp_rc_t;

typedef enum {
	HPGP_MMTYPE_REQ, /*< Management message request */
	HPGP_MMTYPE_CNF, /*< Management message confirm */
	HPGP_MMTYPE_IND, /*< Management message indication */
	HPGP_MMTYPE_RSP, /*< Management message response */
} hpgp_mmtype_variant_t;

typedef enum {
	HPGP_MMTYPE_STA_CCO, /*< messages exchanged between STA and CCo */
	HPGP_MMTYPE_PROXY, /*< messages exchanged with the proxy coordinator */
	HPGP_MMTYPE_CCO_CCO, /*< messages exchanged between neighboring CCos */
	HPGP_MMTYPE_STA_STA, /*< messages exchanged between two stations */
	HPGP_MMTYPE_MANUFACTURE,
	HPGP_MMTYPE_VENDOR,
} hpgp_mmtype_msb_t;

typedef enum {
	HPGP_MMTYPE_DISCOVER_LIST,
	HPGP_MMTYPE_ENCRYPTED,
	HPGP_MMTYPE_SET_KEY,
	HPGP_MMTYPE_GET_KEY,
	HPGP_MMTYPE_BRG_INFO,
	HPGP_MMTYPE_NW_INFO,
	HPGP_MMTYPE_HFID,
	HPGP_MMTYPE_NW_STATS,
	HPGP_MMTYPE_SLAC_PARM,
	HPGP_MMTYPE_START_ATTEN_CHAR,
	HPGP_MMTYPE_ATTEN_CHAR,
	HPGP_MMTYPE_PKCS_CERT,
	HPGP_MMTYPE_MNBC_SOUND,
	HPGP_MMTYPE_VALIDATE,
	HPGP_MMTYPE_SLAC_MATCH,
	HPGP_MMTYPE_SLAC_USER_DATA,
	HPGP_MMTYPE_ATTEN_PROFILE,
	HPGP_MMTYPE_MAX,
} hpgp_mmtype_t;

struct hpgp_frame {
	uint8_t mmv;     /*< Management Message Version */
	uint16_t mmtype; /*< Management Message Type */
	uint8_t body[];
} __attribute__((packed));

struct hpgp_mme_req {
	union {
		struct hpgp_mme_setkey_req setkey;
		struct hpgp_mme_getkey_req getkey;
		struct hpgp_mme_slac_parm_req slac_parm;
	} msg;
};

struct hpgp_mme_cnf {
	union {
		struct hpgp_mme_slac_parm_cnf slac_parm;
		struct hpgp_mme_slac_match_cnf slac_match;
		struct hpgp_mme_getkey_cnf getkey;
	} msg;
};

struct hpgp_mme_ind {
	union {
		struct hpgp_mme_atten_char_ind atten_char;
	} msg;
};

size_t hpgp_pack_request(hpgp_mmtype_t type, const void *req,
		void *buf, size_t bufsize);
size_t hpgp_pack_confirm(hpgp_mmtype_t type, const void *cnf,
		void *buf, size_t bufsize);
size_t hpgp_pack_indication(hpgp_mmtype_t type, const void *ind,
		void *buf, size_t bufsize);
size_t hpgp_pack_response(hpgp_mmtype_t type, const void *rsp,
		void *buf, size_t bufsize);

hpgp_mmtype_t hpgp_mmtype(const struct hpgp_frame *frame);
hpgp_mmtype_variant_t hpgp_mmtype_variant(const struct hpgp_frame *frame);

#if defined(__cplusplus)
}
#endif

#endif /* HPGP_H */
