#ifndef __VSFUSB_H__
#define __VSFUSB_H__

#include "vsfusbh_cfg.h"
#include "vsfusbd_cfg.h"

#if VSFUSBH_ENABLE == 1
#include "stack/usb/core/vsfusbh.h"
#endif

#if VSFUSBD_ENABLE == 1
#include "stack/usb/core/vsfusbd.h"
#endif

#endif // __VSFUSB_H__

