/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <fuse.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#define FSL_SIP_OTP_READ             0xc200000A
#define FSL_SIP_OTP_WRITE            0xc200000B

static bool allow_prog = false;

void fuse_allow_prog(bool allow)
{
	allow_prog = allow;
}

bool fuse_is_prog_allowed(void)
{
	return allow_prog;
}

int fuse_read(u32 bank, u32 word, u32 *val)
{
	return fuse_sense(bank, word, val);
}

int fuse_sense(u32 bank, u32 word, u32 *val)
{
	if (bank != 0) {
		printf("Invalid bank argument, ONLY bank 0 is supported\n");
		return -EINVAL;
	}
#if defined(CONFIG_SMC_FUSE)
	unsigned long ret, value;
	ret = call_imx_sip_ret2(FSL_SIP_OTP_READ, (unsigned long)word,
		&value, 0, 0);
	*val = (u32)value;
	return ret;
#else
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_misc_otp_fuse_read(ipc, word, val);
	if (err != SC_ERR_NONE) {
		printf("fuse read error: %d\n", err);
		return -EIO;
	}

	return 0;
#endif
}

int fuse_prog(u32 bank, u32 word, u32 val)
{
#if CONFIG_MX8_FUSE_PROG
	if (!fuse_is_prog_allowed()) {
		printf("Fuse programming in U-Boot is disabled\n");
		return -EPERM;
	}

	if (bank != 0) {
		printf("Invalid bank argument, ONLY bank 0 is supported\n");
		return -EINVAL;
	}

#if defined(CONFIG_SMC_FUSE)
	return call_imx_sip(FSL_SIP_OTP_WRITE, (unsigned long)word,\
		(unsigned long)val, 0, 0);
#else
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_misc_otp_fuse_write(ipc, word, val);
	if (err != SC_ERR_NONE) {
		printf("fuse write error: %d\n", err);
		return -EIO;
	}

	return 0;
#endif
#endif
	printf("Program fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
}

int fuse_override(u32 bank, u32 word, u32 val)
{
	printf("Override fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
}
