/* The D0/D1 toggle bits ... USE WITH CAUTION (they're almost hcd-internal) */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> (ep)) & 1)
#define	usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << (ep)))
#define usb_settoggle(dev, ep, out, bit) \
		((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << (ep))) | \
		 ((bit) << (ep)))
			
/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)			(((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out)		((dev)->halted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out)	((dev)->halted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out)	((dev)->halted[out] & (1 << (ep)))
