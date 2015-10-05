#ifndef __VSFIP_DNSC_H_INCLUDED__
#define __VSFIP_DNSC_H_INCLUDED__

struct vsfip_hostent_t
{
	uint8_t*	h_name;			/* official name of host */
	uint8_t**	h_aliases;		/* alias list */
	uint32_t	h_addrtype;		/* host address type */
	uint32_t	h_length;		/* length of address */
	uint8_t**	h_addr_list;	/* list of addresses */
};

vsf_err_t vsfip_dns_init(void);
vsf_err_t vsfip_dns_setserver(uint8_t numdns, struct vsfip_ipaddr_t *dnsserver);
vsf_err_t vsfip_gethostbyname(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							uint8_t *domain, struct vsfip_ipaddr_t  *domainip);

#endif		// __VSFIP_DNSC_H_INCLUDED__
