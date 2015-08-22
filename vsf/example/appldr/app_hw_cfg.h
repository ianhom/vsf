#define APP_BOARD_NAME				"led_demo"

struct app_hwcfg_t
{
	char *board;
	struct
	{
		struct
		{
			uint8_t port;
			uint8_t pin;
		} usb_pullup;
	} usbd;
	struct led_hw_t
	{
		uint8_t hport;
		uint8_t hpin;
		uint8_t lport;
		uint8_t lpin;
	} led[24];
};
