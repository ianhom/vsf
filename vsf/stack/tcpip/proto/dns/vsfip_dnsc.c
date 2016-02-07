/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "vsf.h"

#define VSFIP_DNS_CLIENT_PORT			53
#define VSFIP_DNS_TRY_CNT				3

PACKED_HEAD struct PACKED_MID vsfip_dns_head_t
{
	uint16_t id;
	uint16_t flag;
	uint16_t ques;
	uint16_t answ;
	uint16_t auth;
	uint16_t addrrs;
}; PACKED_TAIL


#define VSFIP_DNS_QTYPE_A		1			// A - IP
#define VSFIP_DNS_QTYPE_NS		2			// NS - NameServer
#define VSFIP_DNS_QTYPE_CNAME	5			// CNAME
#define VSFIP_DNS_QTYPE_SOA		6
#define VSFIP_DNS_QTYPE_WKS		11
#define VSFIP_DNS_QTYPE_PTR		12
#define VSFIP_DNS_QTYPE_HINFO	13
#define VSFIP_DNS_QTYPE_MX		15			// MX - Mail
#define VSFIP_DNS_QTYPE_AAAA	28			// IPV6

PACKED_HEAD struct PACKED_MID vsfip_dns_query_type_t
{
	uint16_t type;
	uint16_t classtype;
}; PACKED_TAIL

PACKED_HEAD struct PACKED_MID vsfip_dns_response_t
{
	uint16_t nameptr;
	uint16_t type;
	uint16_t classtype;
	uint32_t ttl;
	uint16_t len;
}; PACKED_TAIL

#ifndef VSFCFG_STANDALONE_MODULE
static struct vsfip_dns_local_t vsfip_dns;
#endif

#define VSFIP_DNS_PKG_SIZE		512
#define VSFIP_DNS_AFNET			0x0100
#define VSFIP_DNS_AFDOMAINADDR	0x0100

static uint16_t vsfip_dns_parse_name(uint8_t *orgin, uint16_t size)
{
	uint8_t n;
	uint8_t *query = orgin;

	do
	{
		n = *query++;

		/** @see RFC 1035 - 4.1.4. Message compression */
		if ((n & 0xc0) == 0xc0)
		{
			/* Compressed name */
			break;
		}
		else
		{
			/* Not compressed name */
			while (n > 0)
			{
				++query;
				--n;
			};
		}
	}
	while (*query != 0);

	return query + 1 - orgin;
}

struct vsfip_buffer_t *vsfip_dns_build_query(char *domain, uint16_t id)
{
	struct vsfip_buffer_t *buf;
	struct vsfip_dns_head_t *head;
	struct vsfip_dns_query_type_t *end;
	uint8_t *name;
	uint16_t count;

	buf = VSFIP_UDPBUF_GET(VSFIP_DNS_PKG_SIZE);
	if (NULL == buf)
	{
		return NULL;
	}

	// fill header
	head = (struct vsfip_dns_head_t *)buf->app.buffer;
	head->id = id;
	head->flag = 0x0001;// unknow
	head->ques = 0x100;
	head->answ = 0;
	head->auth = 0;
	head->addrrs = 0;
	name = (uint8_t *)head + sizeof(struct vsfip_dns_head_t);
	// fill name array
	// empty next size
	count = 0;
	name++;

	while ((*domain != '\0') && (*domain != '/'))
	{
		if (*domain == '.')
		{
			// fill the size
			*(name - count - 1) = count;
			// empty next size
			name++;
			domain++;
			count = 0;
		}
		else
		{
			*name = *domain;
			name++;
			domain++;
			count++;
		}
	}

	*name = 0;
	// fill last size
	// fill the size
	*(name - count - 1) = count;
	name++;
	// finish
	end = (struct vsfip_dns_query_type_t *)name;
	end->type = VSFIP_DNS_AFDOMAINADDR;
	end->classtype = VSFIP_DNS_AFNET;
	buf->app.size = name - (uint8_t *)head +
						sizeof(struct vsfip_dns_query_type_t);
	return buf;
}

vsf_err_t vsfip_dns_decode_ans(uint8_t *ans , uint16_t size, uint16_t id,
								struct vsfip_ipaddr_t *domainip)
{
	struct vsfip_dns_head_t *head = (struct vsfip_dns_head_t *)ans;
	struct vsfip_dns_response_t *atype;
	uint16_t ac;
	uint8_t	*pdat;
	uint16_t dsize;
	uint16_t cache;

	if ((size < sizeof(struct vsfip_dns_head_t)) || (head->id != id))
	{
		return VSFERR_FAIL;
	}

//	qc = SYS_TO_BE_U16(head->ques);
	ac = SYS_TO_BE_U16(head->answ);
	pdat = (uint8_t *) ans + sizeof(struct vsfip_dns_head_t);
	dsize = size - sizeof(struct vsfip_dns_head_t);
	// Skip the name in the "question" part
	cache = vsfip_dns_parse_name(pdat, dsize);

	if (dsize < cache)
	{
		return VSFERR_FAIL;
	}

	pdat += cache;
	dsize -= cache;

	// skip type class
	if (dsize < sizeof(struct vsfip_dns_query_type_t))
	{
		return VSFERR_FAIL;
	}

	pdat += sizeof(struct vsfip_dns_query_type_t);
	dsize -= sizeof(struct vsfip_dns_query_type_t);

	for (uint16_t i = 0 ; i < ac ; i++)
	{
		// skip ansdomain
		atype = (struct vsfip_dns_response_t *)pdat;

		if (dsize < sizeof(struct vsfip_dns_response_t))
		{
			return VSFERR_FAIL;
		}

		// type err next
		if ((atype->classtype != VSFIP_DNS_AFNET) ||
			(atype->type != VSFIP_DNS_AFDOMAINADDR))
		{
			cache = sizeof(struct vsfip_dns_response_t) +
						SYS_TO_BE_U16(atype->len);

			if (dsize < cache)
			{
				return VSFERR_FAIL;
			}

			pdat += cache;
			dsize -= cache;
			continue;
		}

		// no need ttl
		// get ip size
		domainip->size = SYS_TO_BE_U16(atype->len);
		if (dsize < sizeof(struct vsfip_dns_response_t) + domainip->size)
		{
			return VSFERR_FAIL;
		}

		pdat += sizeof(struct vsfip_dns_response_t);

		// copy addr
		if (domainip->size == 4)
		{
			domainip->addr.s_addr = * (uint32_t *) pdat;
			return VSFERR_NONE;
		}
	}

	return VSFERR_FAIL;
}

vsf_err_t vsfip_dns_setserver(uint8_t numdns, struct vsfip_ipaddr_t *dnsserver)
{
	if (numdns < dimof(vsfip_dns.server))
		vsfip_dns.server[numdns] = *dnsserver;
	else
		return VSFERR_NOT_AVAILABLE;

	return VSFERR_NONE;
}

vsf_err_t vsfip_dns_init(void)
{
	return vsfsm_crit_init(&vsfip_dns.crit, VSFSM_EVT_USER_LOCAL);
}

vsf_err_t vsfip_gethostbyname(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							char *domain, struct vsfip_ipaddr_t *domainip)
{
	vsf_err_t err;
	uint8_t i;

	vsfsm_pt_begin(pt);

	if (vsfsm_crit_enter(&vsfip_dns.crit, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
	}

	vsfip_dns.so = vsfip_socket(AF_INET, IPPROTO_UDP);
	if (vsfip_dns.so == NULL)
	{
		err = VSFERR_NOT_AVAILABLE;
		goto close;
	}

	err = vsfip_listen(vsfip_dns.so, 0);
	if (err < 0) goto close;

	vsfip_dns.id++;
	vsfip_dns.outbuf = vsfip_dns_build_query(domain, vsfip_dns.id);
	vsfip_dns.socket_pt.sm = pt->sm;
	vsfip_dns.so->rx_timeout_ms = 1000;

	vsfip_dns.try_cnt = VSFIP_DNS_TRY_CNT;
	do
	{
		for (i = 0 ; i < dimof(vsfip_dns.server) ; i++)
		{
			vsfip_dns.dnsaddr.sin_port = VSFIP_DNS_CLIENT_PORT;
			vsfip_dns.dnsaddr.sin_addr = vsfip_dns.server[i];
			vsfip_dns.socket_pt.state = 0;
			vsfsm_pt_entry(pt);
			vsfip_udp_send(NULL, 0, vsfip_dns.so, &vsfip_dns.dnsaddr,
							vsfip_dns.outbuf);

			// receive
			vsfip_dns.socket_pt.state = 0;
			vsfsm_pt_entry(pt);
			err = vsfip_udp_recv(&vsfip_dns.socket_pt, evt, vsfip_dns.so,
									&vsfip_dns.dnsaddr, &vsfip_dns.inbuf);
			if (err > 0) return err; else if (err < 0) continue;

			// recv success
			err = vsfip_dns_decode_ans(vsfip_dns.inbuf->app.buffer,
							vsfip_dns.inbuf->app.size, vsfip_dns.id, domainip);
			vsfip_buffer_release(vsfip_dns.inbuf);
			if (!err) break;
		}
		vsfip_dns.try_cnt--;
	} while ((err != 0) && (vsfip_dns.try_cnt > 0));

close:
	if (vsfip_dns.outbuf != NULL)
	{
		vsfip_buffer_release(vsfip_dns.outbuf);
		vsfip_dns.outbuf = NULL;
	}
	if (vsfip_dns.so != NULL)
	{
		vsfip_close(vsfip_dns.so);
		vsfip_dns.so = NULL;
	}
	vsfsm_crit_leave(&vsfip_dns.crit);

	vsfsm_pt_end(pt);
	return err;
}
