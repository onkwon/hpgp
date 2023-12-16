/*
 * SPDX-FileCopyrightText: 2023 Kyunghwan Kwon <k@mononn.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "hpgp.h"
#include <string.h>

#define MMTYPE_OFFSET_BIT		2
#define ETH_HEADER_OFFSET		(6/*ODA*/ + 6/*OSA*/ + 2/*EtherType*/)
#define MME_PAD_BOUND_BYTES		(60 - ETH_HEADER_OFFSET)

#if !defined(MIN)
#define MIN(a, b)			(((a) > (b))? (b) : (a))
#endif

typedef size_t (*func_table_t)(struct hpgp_mme *mme,
		const void *data, size_t maxlen);

static void set_mmver(struct hpgp_frame *frame, uint8_t mmv)
{
	frame->mmv = mmv;
}

static void set_mmtype(struct hpgp_frame *frame, uint16_t mmtype)
{
	frame->mmtype = mmtype;
}

static void set_fmi(struct hpgp_mme *mme)
{
	mme->fmi = 0;
	mme->fmi_opt = 0;
}

static void set_header(struct hpgp_frame *frame, uint16_t mmtype)
{
	struct hpgp_mme *mme = (struct hpgp_mme *)frame->body;

	set_mmver(frame, 1);
	set_mmtype(frame, mmtype);

	memset(mme, 0, sizeof(*mme));

	set_fmi(mme);
}

static uint16_t mmtype_to_mmcode(hpgp_mmtype_t type)
{
	uint16_t base = HPGP_MMTYPE_STA_STA << HPGP_MMTYPE_MSB_BIT;
	uint16_t offset = (uint16_t)type;

	if (type >= HPGP_MMTYPE_MAX) {
		return 0;
	} else if (type == HPGP_MMTYPE_DISCOVER_LIST) {
		base = HPGP_MMTYPE_STA_CCO;
		offset = 0x05;
	} else if (type >= HPGP_MMTYPE_SLAC_PARM) {
		offset = (uint16_t)(0x19 + type - HPGP_MMTYPE_SLAC_PARM);
	} else if (type >= HPGP_MMTYPE_NW_STATS) {
		offset = (uint16_t)(0x12 + type - HPGP_MMTYPE_NW_STATS);
	} else if (type >= HPGP_MMTYPE_HFID) {
		offset = (uint16_t)(0x10 + type - HPGP_MMTYPE_HFID);
	} else if (type >= HPGP_MMTYPE_NW_INFO) {
		offset = (uint16_t)(0x0E + type - HPGP_MMTYPE_NW_INFO);
	} else if (type >= HPGP_MMTYPE_BRG_INFO) {
		offset = (uint16_t)(0x08 + type - HPGP_MMTYPE_BRG_INFO);
	}

	return base + (uint16_t)(offset << MMTYPE_OFFSET_BIT);
}

static hpgp_mmtype_t mmcode_to_mmtype(uint16_t code)
{
	hpgp_mmtype_msb_t msb = (hpgp_mmtype_msb_t)
		(code >> HPGP_MMTYPE_MSB_BIT);
	uint16_t offset = (uint16_t)((code >> MMTYPE_OFFSET_BIT) & 0x7ff);

	switch (msb) {
	case HPGP_MMTYPE_STA_CCO:
		if (offset == 5) {
			return HPGP_MMTYPE_DISCOVER_LIST;
		}
		break;
	case HPGP_MMTYPE_STA_STA:
		if (offset >= 0x19) {
			offset = offset - 0x19 + HPGP_MMTYPE_SLAC_PARM;
		} else if (offset >= 0x12) {
			offset = offset - 0x12 + HPGP_MMTYPE_NW_STATS;
		} else if (offset >= 0x10) {
			offset = offset - 0x10 + HPGP_MMTYPE_HFID;
		} else if (offset >= 0x0E) {
			offset = offset - 0x0E + HPGP_MMTYPE_NW_INFO;
		} else if (offset >= 0x08) {
			offset = offset - 0x08 + HPGP_MMTYPE_BRG_INFO;
		}
		return (hpgp_mmtype_t)offset;
		break;
	case HPGP_MMTYPE_VENDOR:
		break;
	case HPGP_MMTYPE_PROXY:
		/* fall through */
	case HPGP_MMTYPE_CCO_CCO:
		/* fall through */
	case HPGP_MMTYPE_MANUFACTURE:
		/* fall through */
	default:
		break;
	}

	return HPGP_MMTYPE_MAX;
}

static size_t pack_nothing(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	(void)mme;
	(void)data;
	(void)maxlen;
	return 0;
}

static size_t copy(void *dst, const void *src, size_t len)
{
	memcpy(dst, src, len);
	return len;
}

static size_t pack_setkey_req(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_setkey_req), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_slac_parm_req(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_slac_parm_req), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_slac_match_req(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_slac_match_req), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_slac_parm_cnf(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_slac_parm_cnf), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_slac_match_cnf(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_slac_match_cnf), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_start_atten_char_ind(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_start_atten_char_ind), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_mnbc_sound_ind(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_mnbc_sound_ind), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_atten_char_ind(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_atten_char_ind), maxlen);
	return copy(mme->data, data, len);
}

static size_t pack_atten_char_rsp(struct hpgp_mme *mme,
		const void *data, size_t maxlen)
{
	const size_t len = MIN(sizeof(struct hpgp_mme_atten_char_rsp), maxlen);
	return copy(mme->data, data, len);
}

static func_table_t req_func_table[] = {
	pack_nothing,			/*HPGP_MMTYPE_DISCOVER_LIST*/
	pack_nothing,			/*HPGP_MMTYPE_ENCRYPTED*/
	pack_setkey_req,		/*HPGP_MMTYPE_SET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_GET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_BRG_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_NW_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_HFID*/
	pack_nothing,			/*HPGP_MMTYPE_NW_STATS*/
	pack_slac_parm_req,		/*HPGP_MMTYPE_SLAC_PARM*/
	pack_nothing,			/*HPGP_MMTYPE_START_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_PKCS_CERT*/
	pack_nothing,			/*HPGP_MMTYPE_MNBC_SOUND*/
	pack_nothing,			/*HPGP_MMTYPE_VALIDATE*/
	pack_slac_match_req,		/*HPGP_MMTYPE_SLAC_MATCH*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_USER_DATA*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_PROFILE*/
};

static func_table_t cnf_func_table[] = {
	pack_nothing,			/*HPGP_MMTYPE_DISCOVER_LIST*/
	pack_nothing,			/*HPGP_MMTYPE_ENCRYPTED*/
	pack_nothing,			/*HPGP_MMTYPE_SET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_GET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_BRG_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_NW_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_HFID*/
	pack_nothing,			/*HPGP_MMTYPE_NW_STATS*/
	pack_slac_parm_cnf,		/*HPGP_MMTYPE_SLAC_PARM*/
	pack_nothing,			/*HPGP_MMTYPE_START_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_PKCS_CERT*/
	pack_nothing,			/*HPGP_MMTYPE_MNBC_SOUND*/
	pack_nothing,			/*HPGP_MMTYPE_VALIDATE*/
	pack_slac_match_cnf,		/*HPGP_MMTYPE_SLAC_MATCH*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_USER_DATA*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_PROFILE*/
};

static func_table_t ind_func_table[] = {
	pack_nothing,			/*HPGP_MMTYPE_DISCOVER_LIST*/
	pack_nothing,			/*HPGP_MMTYPE_ENCRYPTED*/
	pack_nothing,			/*HPGP_MMTYPE_SET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_GET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_BRG_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_NW_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_HFID*/
	pack_nothing,			/*HPGP_MMTYPE_NW_STATS*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_PARM*/
	pack_start_atten_char_ind,	/*HPGP_MMTYPE_START_ATTEN_CHAR*/
	pack_atten_char_ind,		/*HPGP_MMTYPE_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_PKCS_CERT*/
	pack_mnbc_sound_ind,		/*HPGP_MMTYPE_MNBC_SOUND*/
	pack_nothing,			/*HPGP_MMTYPE_VALIDATE*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_MATCH*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_USER_DATA*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_PROFILE*/
};

static func_table_t rsp_func_table[] = {
	pack_nothing,			/*HPGP_MMTYPE_DISCOVER_LIST*/
	pack_nothing,			/*HPGP_MMTYPE_ENCRYPTED*/
	pack_nothing,			/*HPGP_MMTYPE_SET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_GET_KEY*/
	pack_nothing,			/*HPGP_MMTYPE_BRG_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_NW_INFO*/
	pack_nothing,			/*HPGP_MMTYPE_HFID*/
	pack_nothing,			/*HPGP_MMTYPE_NW_STATS*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_PARM*/
	pack_nothing,			/*HPGP_MMTYPE_START_ATTEN_CHAR*/
	pack_atten_char_rsp,		/*HPGP_MMTYPE_ATTEN_CHAR*/
	pack_nothing,			/*HPGP_MMTYPE_PKCS_CERT*/
	pack_nothing,			/*HPGP_MMTYPE_MNBC_SOUND*/
	pack_nothing,			/*HPGP_MMTYPE_VALIDATE*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_MATCH*/
	pack_nothing,			/*HPGP_MMTYPE_SLAC_USER_DATA*/
	pack_nothing,			/*HPGP_MMTYPE_ATTEN_PROFILE*/
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
_Static_assert(sizeof(req_func_table) / sizeof(req_func_table[0]) == HPGP_MMTYPE_MAX, "");
_Static_assert(sizeof(cnf_func_table) / sizeof(cnf_func_table[0]) == HPGP_MMTYPE_MAX, "");
_Static_assert(sizeof(ind_func_table) / sizeof(ind_func_table[0]) == HPGP_MMTYPE_MAX, "");
#pragma GCC diagnostic pop

static size_t pack(hpgp_mmtype_t type, hpgp_mmtype_variant_t variant,
		const void *data, void *buf, size_t bufsize,
		func_table_t *func_table)
{
	struct hpgp_frame *frame_buf = (struct hpgp_frame *)buf;
	struct hpgp_mme *mme = (struct hpgp_mme *)frame_buf->body;
	const size_t frame_header_len = sizeof(*frame_buf) + sizeof(*mme);
	size_t maxlen = MIN(bufsize, bufsize - frame_header_len);

	memset(buf, 0, bufsize);
	set_header(frame_buf, mmtype_to_mmcode(type) | (uint16_t)variant);

	size_t len = (*func_table[type])(mme, data, maxlen);

	if (len && (len + frame_header_len) < MME_PAD_BOUND_BYTES) {
		len = MME_PAD_BOUND_BYTES - frame_header_len;
	}

	return len;
}

size_t hpgp_pack_request(hpgp_mmtype_t type, const void *req,
		void *buf, size_t bufsize)
{
	return pack(type, HPGP_MMTYPE_REQ, req, buf, bufsize, req_func_table);
}

size_t hpgp_pack_confirm(hpgp_mmtype_t type, const void *cnf,
		void *buf, size_t bufsize)
{
	return pack(type, HPGP_MMTYPE_CNF, cnf, buf, bufsize, cnf_func_table);
}

size_t hpgp_pack_indication(hpgp_mmtype_t type, const void *ind,
		void *buf, size_t bufsize)
{
	return pack(type, HPGP_MMTYPE_IND, ind, buf, bufsize, ind_func_table);
}

size_t hpgp_pack_response(hpgp_mmtype_t type, const void *rsp,
		void *buf, size_t bufsize)
{
	return pack(type, HPGP_MMTYPE_RSP, rsp, buf, bufsize, rsp_func_table);
}

hpgp_mmtype_t hpgp_mmtype(const struct hpgp_frame *frame)
{
	return mmcode_to_mmtype(frame->mmtype);
}

hpgp_mmtype_variant_t hpgp_mmtype_variant(
		const struct hpgp_frame *frame)
{
	return (hpgp_mmtype_variant_t)(frame->mmtype & 0x3);
}
