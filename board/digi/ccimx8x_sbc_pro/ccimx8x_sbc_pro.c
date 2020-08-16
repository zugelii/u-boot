/*
 * Copyright (C) 2018 Digi International, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <fsl_esdhc.h>

#include <asm/gpio.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <usb.h>
#include <cdns3-uboot.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <power-domain.h>
#include "../../freescale/common/tcpc.h"

#include "../ccimx8x/ccimx8x.h"
#include "../common/carrier_board.h"
#include "../common/helper.h"
#include "../common/hwid.h"
#include "../common/mca_registers.h"
#include "../common/mca.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned int board_version = CARRIERBOARD_VERSION_UNDEFINED;
unsigned int board_id = CARRIERBOARD_ID_UNDEFINED;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define FSPI_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define I2C_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

static iomux_cfg_t uart2_pads[] = {
	SC_P_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart2_pads, ARRAY_SIZE(uart2_pads));
}

int board_early_init_f(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up UART2 */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_2, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART2 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_2, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART2 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_2, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FEC_MXC
#include <miiphy.h>

static void enet_device_phy_reset(void)
{
	struct gpio_desc desc;
	struct udevice *dev = NULL;
	char *reset_gpio[] = { "gpio3_18", "gpio3_22" };
	char *reset_gpio_lbl[] = { "enet0_reset", "enet1_reset" };
	int iface = CONFIG_FEC_ENET_DEV;
	int ret;

	if (iface > 1) {
		printf("Error: invalid CONFIG_FEC_ENET_DEV\n");
		return;
	}

	ret = dm_gpio_lookup_name(reset_gpio[iface], &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, reset_gpio_lbl[iface]);
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(50);
	dm_gpio_set_value(&desc, 1);
	dm_gpio_free(dev, &desc);

	udelay(10);
}

int board_phy_config(struct phy_device *phydev)
{
	/* Set RGMII IO voltage to 1.8V */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	/* Introduce RGMII RX clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);

	/* Introduce RGMII TX clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static int setup_fec(int ind)
{
	struct gpio_desc enet_pwr;
	int ret;

	/* Power up the PHY */
	ret = dm_gpio_lookup_name("gpio3_13", &enet_pwr);
	if (ret)
		return -1;

	ret = dm_gpio_request(&enet_pwr, "enet0_pwr");
	if (ret)
		return -1;

	dm_gpio_set_dir_flags(&enet_pwr, GPIOD_IS_OUT);
	dm_gpio_set_value(&enet_pwr, 1);
	mdelay(1);	/* PHY power up time */

	/* Reset ENET PHY */
	enet_device_phy_reset();

	return 0;
}
#endif

#ifdef CONFIG_MXC_GPIO
static iomux_cfg_t board_gpios[] = {
};

static void board_gpio_init(void)
{
	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));
}
#endif

int checkboard(void)
{
	board_version = get_carrierboard_version();
	board_id = get_carrierboard_id();
	printf("board_version: %d board_id : %d\r\n", board_version, board_id);

	print_ccimx8x_info();
	print_carrierboard_info();
	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH)
		printf("\nSCI error! Invalid handle\n");

#ifdef SCI_FORCE_ABORT
	sc_rpc_msg_t abort_msg;

	puts("Send abort request\n");
	RPC_SIZE(&abort_msg) = 1;
	RPC_SVC(&abort_msg) = SC_RPC_SVC_ABORT;
	sc_ipc_write(SC_IPC_CH, &abort_msg);

	/* Close IPC channel */
	sc_ipc_close(SC_IPC_CH);
#endif /* SCI_FORCE_ABORT */

	return 0;
}

#ifdef CONFIG_USB

#ifdef CONFIG_USB_TCPC
#define USB_TYPEC_SEL IMX_GPIO_NR(4, 6)
#define USB_TYPEC_EN IMX_GPIO_NR(4, 5)

static iomux_cfg_t ss_gpios[] = {
	SC_P_USB_SS3_TC3 | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_USB_SS3_TC2 | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

struct tcpc_port port;
struct tcpc_port_config port_config = {
	.i2c_bus = 3,
	.addr = 0x52,
	.port_type = TYPEC_PORT_DFP,
};

void ss_mux_select(enum typec_cc_polarity pol)
{
	if (pol == TYPEC_POLARITY_CC1)
		gpio_direction_output(USB_TYPEC_SEL, 0);
	else
		gpio_direction_output(USB_TYPEC_SEL, 1);
}

static void setup_typec(void)
{
	imx8_iomux_setup_multiple_pads(ss_gpios, ARRAY_SIZE(ss_gpios));
	gpio_request(USB_TYPEC_SEL, "typec_sel");
	/* USB_SS3_SW_PWR (Active LOW) */
	gpio_request(USB_TYPEC_EN, "typec_en");
	gpio_direction_output(USB_TYPEC_EN, 0);

	tcpc_init(&port, port_config, &ss_mux_select);
}
#endif

#ifdef CONFIG_USB_CDNS3_GADGET
static struct cdns3_device cdns3_device_data = {
	.none_core_base = 0x5B110000,
	.xhci_base = 0x5B130000,
	.dev_base = 0x5B140000,
	.phy_base = 0x5B160000,
	.otg_base = 0x5B120000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 1,
};

int usb_gadget_handle_interrupts(void)
{
	cdns3_uboot_handle_interrupt(1);
	return 0;
}
#endif

static iomux_cfg_t usb_hub_gpio = {
	SC_P_SPI3_SDI | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL)
};

static int setup_usb_hub(void)
{
	struct gpio_desc usb_pwr;
	int ret;

	imx8_iomux_setup_pad(usb_hub_gpio);

	/* Power up the USB hub */
	ret = dm_gpio_lookup_name("gpio0_15", &usb_pwr);
	if (ret)
		return -1;

	ret = dm_gpio_request(&usb_pwr, "usb_pwr");
	if (ret)
		return -1;

	dm_gpio_set_dir_flags(&usb_pwr, GPIOD_IS_OUT);
	dm_gpio_set_value(&usb_pwr, 1);

	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
			ret = tcpc_setup_dfp_mode(&port);
#endif
		} else {
#ifdef CONFIG_USB_CDNS3_GADGET
			struct power_domain pd;

			/* Power on usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}
#ifdef CONFIG_USB_TCPC
			ret = tcpc_setup_ufp_mode(&port);
#endif
			ret = cdns3_uboot_init(&cdns3_device_data);
#endif
		}
	}

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
			ret = tcpc_disable_src_vbus(&port);
#endif
		} else {
#ifdef CONFIG_USB_CDNS3_GADGET
			struct power_domain pd;

			cdns3_uboot_exit(1);

			/* Power off usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2 Power down failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2_phy Power down failed! (error = %d)\n", ret);
			}
#endif
		}
	}

	return ret;
}
#endif

int board_power_led_init(void)
{
	/* MCA_IO13 (bank 1, bit 5) is connected to POWER_LED */
	int prw_led_gpiobank = 1;
	int pwr_led_gpiobit = (1 << 5);
	int ret;

	/* Configure as output */
	ret = mca_update_bits(MCA_GPIO_DIR_0 + prw_led_gpiobank, pwr_led_gpiobit, pwr_led_gpiobit);
	if (ret != 0)
		return ret;

	/* Turn on POWER_LED (high) */
	ret = mca_update_bits(MCA_GPIO_DATA_0 + prw_led_gpiobank, pwr_led_gpiobit, pwr_led_gpiobit);
	return ret;
}

int board_init(void)
{
	/* SOM init */
	ccimx8_init();

	//board_power_led_init();

#ifdef CONFIG_MXC_GPIO
	board_gpio_init();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#if defined(CONFIG_USB)
	setup_usb_hub();
#if defined(CONFIG_USB_TCPC)
	setup_typec();
#endif
#endif

	return 0;
}

void board_quiesce_devices()
{
	const char *power_on_devices[] = {
		"dma_lpuart2",

		/* HIFI DSP boot */
		"audio_sai0",
		"audio_ocram",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

#if defined(CONFIG_OF_BOARD_SETUP)
/* Platform function to modify the FDT as needed */
int ft_board_setup(void *blob, bd_t *bd)
{
	fdt_fixup_hwid(blob);
	fdt_fixup_ccimx8x(blob);
	fdt_fixup_carrierboard(blob);

	return 0;
}
#endif /* CONFIG_OF_BOARD_SETUP */

void platform_default_environment(void)
{
	som_default_environment();
}

int board_late_init(void)
{
	/* SOM late init */
	ccimx8x_late_init();

	/* Set default dynamic variables */
	platform_default_environment();

	return 0;
}
