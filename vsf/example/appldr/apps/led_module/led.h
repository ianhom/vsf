
#ifndef __LED_H_INCLUDED__
#define __LED_H_INCLUDED__

struct led_t
{
	struct led_hw_t const *hw;
	bool on;
};

struct led_ifs_t
{
	vsf_err_t(*init)(struct led_t *led);
	vsf_err_t(*on)(struct led_t *led);
	vsf_err_t(*off)(struct led_t *led);
	bool(*is_on)(struct led_t *led);
};

#endif		// __LED_H_INCLUDED__
