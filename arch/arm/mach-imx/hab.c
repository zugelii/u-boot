/*
 * Copyright (C) 2010-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <fuse.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/hab.h>

DECLARE_GLOBAL_DATA_PTR;

#define ALIGN_SIZE		0x1000
#define MX6DQ_PU_IROM_MMU_EN_VAR	0x009024a8
#define MX6DLS_PU_IROM_MMU_EN_VAR	0x00901dd0
#define MX6SL_PU_IROM_MMU_EN_VAR	0x00900a18
#define IS_HAB_ENABLED_BIT \
	(is_soc_type(MXC_SOC_MX7ULP) ? 0x80000000 :	\
	 ((is_soc_type(MXC_SOC_MX7) || is_soc_type(MXC_SOC_IMX8M))? 0x2000000 : 0x2))

static int ivt_header_error(const char *err_str, struct ivt_header *ivt_hdr)
{
	printf("%s magic=0x%x length=0x%02x version=0x%x\n", err_str,
	       ivt_hdr->magic, ivt_hdr->length, ivt_hdr->version);

	return 1;
}

static int verify_ivt_header(struct ivt_header *ivt_hdr)
{
	int result = 0;

	if (ivt_hdr->magic != IVT_HEADER_MAGIC)
		result = ivt_header_error("bad magic", ivt_hdr);

	if (be16_to_cpu(ivt_hdr->length) != IVT_TOTAL_LENGTH)
		result = ivt_header_error("bad length", ivt_hdr);

	if (ivt_hdr->version != IVT_HEADER_V1 &&
	    ivt_hdr->version != IVT_HEADER_V2)
		result = ivt_header_error("bad version", ivt_hdr);

	return result;
}

#ifdef CONFIG_ARM64
#define FSL_SIP_HAB		0xC2000007
#define FSL_SIP_HAB_AUTHENTICATE	0x00
#define FSL_SIP_HAB_ENTRY		0x01
#define FSL_SIP_HAB_EXIT		0x02
#define FSL_SIP_HAB_REPORT_EVENT	0x03
#define FSL_SIP_HAB_REPORT_STATUS	0x04
#define FSL_SIP_HAB_FAILSAFE		0x05
#define FSL_SIP_HAB_CHECK_TARGET	0x06
static volatile gd_t *gd_save;
#endif

static inline void save_gd(void)
{
#ifdef CONFIG_ARM64
	gd_save = gd;
#endif
}

static inline void restore_gd(void)
{
#ifdef CONFIG_ARM64
	/*
	 * Make will already error that reserving x18 is not supported at the
	 * time of writing, clang: error: unknown argument: '-ffixed-x18'
	 */
	__asm__ volatile("mov x18, %0\n" : : "r" (gd_save));
#endif
}

enum hab_status hab_rvt_report_event(enum hab_status status, uint32_t index,
		uint8_t *event, size_t *bytes)
{
	enum hab_status ret;
	hab_rvt_report_event_t *hab_rvt_report_event_func;
	hab_rvt_report_event_func =  (hab_rvt_report_event_t *)HAB_RVT_REPORT_EVENT;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (enum hab_status)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_REPORT_EVENT, (unsigned long)index,
			(unsigned long)event, (unsigned long)bytes);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_report_event_func(status, index, event, bytes);
	restore_gd();

	return ret;

}

enum hab_status hab_rvt_report_status(enum hab_config *config,
		enum hab_state *state)
{
	enum hab_status ret;
	hab_rvt_report_status_t *hab_rvt_report_status_func;
	hab_rvt_report_status_func = (hab_rvt_report_status_t *)HAB_RVT_REPORT_STATUS;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (enum hab_status)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_REPORT_STATUS,
			(unsigned long)config, (unsigned long)state, 0);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_report_status_func(config, state);
	restore_gd();

	return ret;
}

enum hab_status hab_rvt_entry(void)
{
	enum hab_status ret;
	hab_rvt_entry_t *hab_rvt_entry_func;
	hab_rvt_entry_func = (hab_rvt_entry_t *)HAB_RVT_ENTRY;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (enum hab_status)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_ENTRY, 0, 0, 0);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_entry_func();
	restore_gd();

	return ret;
}

enum hab_status hab_rvt_exit(void)
{
	enum hab_status ret;
	hab_rvt_exit_t *hab_rvt_exit_func;
	hab_rvt_exit_func =  (hab_rvt_exit_t *)HAB_RVT_EXIT;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (enum hab_status)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_EXIT, 0, 0, 0);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_exit_func();
	restore_gd();

	return ret;
}

void hab_rvt_failsafe(void)
{
	hab_rvt_failsafe_t *hab_rvt_failsafe_func;
	hab_rvt_failsafe_func = (hab_rvt_failsafe_t *)HAB_RVT_FAILSAFE;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_FAILSAFE, 0, 0, 0);
		return;
	}
#endif

	save_gd();
	hab_rvt_failsafe_func();
	restore_gd();
}

enum hab_status hab_rvt_check_target(enum hab_target type, const void *start,
					       size_t bytes)
{
	enum hab_status ret;
	hab_rvt_check_target_t *hab_rvt_check_target_func;
	hab_rvt_check_target_func =  (hab_rvt_check_target_t *)HAB_RVT_CHECK_TARGET;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (enum hab_status)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_CHECK_TARGET, (unsigned long)type,
			(unsigned long)start, (unsigned long)bytes);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_check_target_func(type, start, bytes);
	restore_gd();

	return ret;
}

void *hab_rvt_authenticate_image(uint8_t cid, ptrdiff_t ivt_offset,
		void **start, size_t *bytes, hab_loader_callback_f_t loader)
{
	void *ret;
	hab_rvt_authenticate_image_t *hab_rvt_authenticate_image_func;
	hab_rvt_authenticate_image_func = (hab_rvt_authenticate_image_t *)HAB_RVT_AUTHENTICATE_IMAGE;

#if defined(CONFIG_ARM64)
	if (current_el() != 3) {
		/* call sip */
		ret = (void *)call_imx_sip(FSL_SIP_HAB, FSL_SIP_HAB_AUTHENTICATE, (unsigned long)ivt_offset,
			(unsigned long)start, (unsigned long)bytes);
		return ret;
	}
#endif

	save_gd();
	ret = hab_rvt_authenticate_image_func(cid, ivt_offset, start, bytes, loader);
	restore_gd();

	return ret;
}

#if !defined(CONFIG_SPL_BUILD)

#define MAX_RECORD_BYTES     (8*1024) /* 4 kbytes */

struct record {
	uint8_t  tag;						/* Tag */
	uint8_t  len[2];					/* Length */
	uint8_t  par;						/* Version */
	uint8_t  contents[MAX_RECORD_BYTES];/* Record Data */
	bool	 any_rec_flag;
};

static char *rsn_str[] = {
			  "RSN = HAB_RSN_ANY (0x00)\n",
			  "RSN = HAB_ENG_FAIL (0x30)\n",
			  "RSN = HAB_INV_ADDRESS (0x22)\n",
			  "RSN = HAB_INV_ASSERTION (0x0C)\n",
			  "RSN = HAB_INV_CALL (0x28)\n",
			  "RSN = HAB_INV_CERTIFICATE (0x21)\n",
			  "RSN = HAB_INV_COMMAND (0x06)\n",
			  "RSN = HAB_INV_CSF (0x11)\n",
			  "RSN = HAB_INV_DCD (0x27)\n",
			  "RSN = HAB_INV_INDEX (0x0F)\n",
			  "RSN = HAB_INV_IVT (0x05)\n",
			  "RSN = HAB_INV_KEY (0x1D)\n",
			  "RSN = HAB_INV_RETURN (0x1E)\n",
			  "RSN = HAB_INV_SIGNATURE (0x18)\n",
			  "RSN = HAB_INV_SIZE (0x17)\n",
			  "RSN = HAB_MEM_FAIL (0x2E)\n",
			  "RSN = HAB_OVR_COUNT (0x2B)\n",
			  "RSN = HAB_OVR_STORAGE (0x2D)\n",
			  "RSN = HAB_UNS_ALGORITHM (0x12)\n",
			  "RSN = HAB_UNS_COMMAND (0x03)\n",
			  "RSN = HAB_UNS_ENGINE (0x0A)\n",
			  "RSN = HAB_UNS_ITEM (0x24)\n",
			  "RSN = HAB_UNS_KEY (0x1B)\n",
			  "RSN = HAB_UNS_PROTOCOL (0x14)\n",
			  "RSN = HAB_UNS_STATE (0x09)\n",
			  "RSN = INVALID\n",
			  NULL
};

static char *sts_str[] = {
			  "STS = HAB_SUCCESS (0xF0)\n",
			  "STS = HAB_FAILURE (0x33)\n",
			  "STS = HAB_WARNING (0x69)\n",
			  "STS = INVALID\n",
			  NULL
};

static char *eng_str[] = {
			  "ENG = HAB_ENG_ANY (0x00)\n",
			  "ENG = HAB_ENG_SCC (0x03)\n",
			  "ENG = HAB_ENG_RTIC (0x05)\n",
			  "ENG = HAB_ENG_SAHARA (0x06)\n",
			  "ENG = HAB_ENG_CSU (0x0A)\n",
			  "ENG = HAB_ENG_SRTC (0x0C)\n",
			  "ENG = HAB_ENG_DCP (0x1B)\n",
			  "ENG = HAB_ENG_CAAM (0x1D)\n",
			  "ENG = HAB_ENG_SNVS (0x1E)\n",
			  "ENG = HAB_ENG_OCOTP (0x21)\n",
			  "ENG = HAB_ENG_DTCP (0x22)\n",
			  "ENG = HAB_ENG_ROM (0x36)\n",
			  "ENG = HAB_ENG_HDCP (0x24)\n",
			  "ENG = HAB_ENG_RTL (0x77)\n",
			  "ENG = HAB_ENG_SW (0xFF)\n",
			  "ENG = INVALID\n",
			  NULL
};

static char *ctx_str[] = {
			  "CTX = HAB_CTX_ANY(0x00)\n",
			  "CTX = HAB_CTX_FAB (0xFF)\n",
			  "CTX = HAB_CTX_ENTRY (0xE1)\n",
			  "CTX = HAB_CTX_TARGET (0x33)\n",
			  "CTX = HAB_CTX_AUTHENTICATE (0x0A)\n",
			  "CTX = HAB_CTX_DCD (0xDD)\n",
			  "CTX = HAB_CTX_CSF (0xCF)\n",
			  "CTX = HAB_CTX_COMMAND (0xC0)\n",
			  "CTX = HAB_CTX_AUT_DAT (0xDB)\n",
			  "CTX = HAB_CTX_ASSERT (0xA0)\n",
			  "CTX = HAB_CTX_EXIT (0xEE)\n",
			  "CTX = INVALID\n",
			  NULL
};

static uint8_t hab_statuses[5] = {
	HAB_STS_ANY,
	HAB_FAILURE,
	HAB_WARNING,
	HAB_SUCCESS,
	-1
};

static uint8_t hab_reasons[26] = {
	HAB_RSN_ANY,
	HAB_ENG_FAIL,
	HAB_INV_ADDRESS,
	HAB_INV_ASSERTION,
	HAB_INV_CALL,
	HAB_INV_CERTIFICATE,
	HAB_INV_COMMAND,
	HAB_INV_CSF,
	HAB_INV_DCD,
	HAB_INV_INDEX,
	HAB_INV_IVT,
	HAB_INV_KEY,
	HAB_INV_RETURN,
	HAB_INV_SIGNATURE,
	HAB_INV_SIZE,
	HAB_MEM_FAIL,
	HAB_OVR_COUNT,
	HAB_OVR_STORAGE,
	HAB_UNS_ALGORITHM,
	HAB_UNS_COMMAND,
	HAB_UNS_ENGINE,
	HAB_UNS_ITEM,
	HAB_UNS_KEY,
	HAB_UNS_PROTOCOL,
	HAB_UNS_STATE,
	-1
};

static uint8_t hab_contexts[12] = {
	HAB_CTX_ANY,
	HAB_CTX_FAB,
	HAB_CTX_ENTRY,
	HAB_CTX_TARGET,
	HAB_CTX_AUTHENTICATE,
	HAB_CTX_DCD,
	HAB_CTX_CSF,
	HAB_CTX_COMMAND,
	HAB_CTX_AUT_DAT,
	HAB_CTX_ASSERT,
	HAB_CTX_EXIT,
	-1
};

static uint8_t hab_engines[16] = {
	HAB_ENG_ANY,
	HAB_ENG_SCC,
	HAB_ENG_RTIC,
	HAB_ENG_SAHARA,
	HAB_ENG_CSU,
	HAB_ENG_SRTC,
	HAB_ENG_DCP,
	HAB_ENG_CAAM,
	HAB_ENG_SNVS,
	HAB_ENG_OCOTP,
	HAB_ENG_DTCP,
	HAB_ENG_ROM,
	HAB_ENG_HDCP,
	HAB_ENG_RTL,
	HAB_ENG_SW,
	-1
};

static inline uint8_t get_idx(uint8_t *list, uint8_t tgt)
{
	uint8_t idx = 0;
	uint8_t element = list[idx];
	while (element != -1) {
		if (element == tgt)
			return idx;
		element = list[++idx];
	}
	return -1;
}

static void process_event_record(uint8_t *event_data, size_t bytes)
{
	struct record *rec = (struct record *)event_data;

	printf("\n\n%s", sts_str[get_idx(hab_statuses, rec->contents[0])]);
	printf("%s", rsn_str[get_idx(hab_reasons, rec->contents[1])]);
	printf("%s", ctx_str[get_idx(hab_contexts, rec->contents[2])]);
	printf("%s", eng_str[get_idx(hab_engines, rec->contents[3])]);
}

static void display_event(uint8_t *event_data, size_t bytes)
{
	uint32_t i;

	if (!(event_data && bytes > 0))
		return;

	for (i = 0; i < bytes; i++) {
		if (i == 0)
			printf("\t0x%02x", event_data[i]);
		else if ((i % 8) == 0)
			printf("\n\t0x%02x", event_data[i]);
		else
			printf(" 0x%02x", event_data[i]);
	}

	process_event_record(event_data, bytes);
}

static int get_hab_status(void)
{
	uint32_t index = 0; /* Loop index */
	uint8_t event_data[128]; /* Event data buffer */
	size_t bytes = sizeof(event_data); /* Event size in bytes */
	enum hab_config config = 0;
	enum hab_state state = 0;

	if (imx_hab_is_enabled())
		puts("\nSecure boot enabled\n");
	else
		puts("\nSecure boot disabled\n");

	/* Check HAB status */
	if (hab_rvt_report_status(&config, &state) != HAB_SUCCESS) {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
		       config, state);

		/* Display HAB Error events */
		while (hab_rvt_report_event(HAB_FAILURE, index, event_data,
					&bytes) == HAB_SUCCESS) {
			puts("\n");
			printf("--------- HAB Event %d -----------------\n",
			       index + 1);
			puts("event data:\n");
			display_event(event_data, bytes);
			puts("\n");
			bytes = sizeof(event_data);
			index++;
		}
	}
	/* Display message if no HAB events are found */
	else {
		printf("\nHAB Configuration: 0x%02x, HAB State: 0x%02x\n",
		       config, state);
		puts("No HAB Events Found!\n\n");
	}
	return 0;
}

static int do_hab_status(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	if ((argc != 1)) {
		cmd_usage(cmdtp);
		return 1;
	}

	get_hab_status();

	return 0;
}

static int do_authenticate_image(cmd_tbl_t *cmdtp, int flag, int argc,
				 char * const argv[])
{
	ulong	addr, length, ivt_offset;
	int	rcode = 0;

	if (argc < 4)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);
	length = simple_strtoul(argv[2], NULL, 16);
	ivt_offset = simple_strtoul(argv[3], NULL, 16);

	rcode = imx_hab_authenticate_image(addr, length, ivt_offset);
	if (rcode == 0)
		rcode = CMD_RET_SUCCESS;
	else
		rcode = CMD_RET_FAILURE;

	return rcode;
}

static int do_hab_failsafe(cmd_tbl_t *cmdtp, int flag, int argc,
			   char * const argv[])
{
	if (argc != 1) {
		cmd_usage(cmdtp);
		return 1;
	}

	hab_rvt_failsafe();

	return 0;
}

U_BOOT_CMD(
		hab_status, CONFIG_SYS_MAXARGS, 1, do_hab_status,
		"display HAB status",
		""
	  );

U_BOOT_CMD(
		hab_auth_img, 4, 0, do_authenticate_image,
		"authenticate image via HAB",
		"addr length ivt_offset\n"
		"addr - image hex address\n"
		"length - image hex length\n"
		"ivt_offset - hex offset of IVT in the image"
	  );

U_BOOT_CMD(
		hab_failsafe, CONFIG_SYS_MAXARGS, 1, do_hab_failsafe,
		"run BootROM failsafe routine",
		""
	  );

#endif /* !defined(CONFIG_SPL_BUILD) */

/* Get CSF Header length */
static int get_hab_hdr_len(struct hab_hdr *hdr)
{
	return (size_t)((hdr->len[0] << 8) + (hdr->len[1]));
}

/* Check whether addr lies between start and
 * end and is within the length of the image
 */
static int chk_bounds(u8 *addr, size_t bytes, u8 *start, u8 *end)
{
	size_t csf_size = (size_t)((end + 1) - addr);

	return (addr && (addr >= start) && (addr <= end) &&
		(csf_size >= bytes));
}

/* Get Length of each command in CSF */
static int get_csf_cmd_hdr_len(u8 *csf_hdr)
{
	if (*csf_hdr == HAB_CMD_HDR)
		return sizeof(struct hab_hdr);

	return get_hab_hdr_len((struct hab_hdr *)csf_hdr);
}

/* Check if CSF is valid */
static bool csf_is_valid(struct ivt *ivt, ulong start_addr, size_t bytes)
{
	u8 *start = (u8 *)start_addr;
	u8 *csf_hdr;
	u8 *end;

	size_t csf_hdr_len;
	size_t cmd_hdr_len;
	size_t offset = 0;

	if (bytes != 0)
		end = start + bytes - 1;
	else
		end = start;

	/* Verify if CSF pointer content is zero */
	if (!ivt->csf) {
		puts("Error: CSF pointer is NULL\n");
		return false;
	}

	csf_hdr = (u8 *)(ulong)ivt->csf;

	/* Verify if CSF Header exist */
	if (*csf_hdr != HAB_CMD_HDR) {
		puts("Error: CSF header command not found\n");
		return false;
	}

	csf_hdr_len = get_hab_hdr_len((struct hab_hdr *)csf_hdr);

	/* Check if the CSF lies within the image bounds */
	if (!chk_bounds(csf_hdr, csf_hdr_len, start, end)) {
		puts("Error: CSF lies outside the image bounds\n");
		return false;
	}

	do {
		struct hab_hdr *cmd;

		cmd = (struct hab_hdr *)&csf_hdr[offset];

		switch (cmd->tag) {
		case (HAB_CMD_WRT_DAT):
			puts("Error: Deprecated write command found\n");
			return false;
		case (HAB_CMD_CHK_DAT):
			puts("Error: Deprecated check command found\n");
			return false;
		case (HAB_CMD_SET):
			if (cmd->par == HAB_PAR_MID) {
				puts("Error: Deprecated Set MID command found\n");
				return false;
			}
		default:
			break;
		}

		cmd_hdr_len = get_csf_cmd_hdr_len(&csf_hdr[offset]);
		if (!cmd_hdr_len) {
			puts("Error: Invalid command length\n");
			return false;
		}
		offset += cmd_hdr_len;

	} while (offset < csf_hdr_len);

	return true;
}

/*
 * Validate IVT structure of the image being authenticated
 */
static int validate_ivt(struct ivt *ivt_initial)
{
	struct ivt_header *ivt_hdr = &ivt_initial->hdr;

	if ((ulong)ivt_initial & 0x3) {
		puts("Error: Image's start address is not 4 byte aligned\n");
		return 0;
	}

	/* Check IVT fields before allowing authentication */
	if ((!verify_ivt_header(ivt_hdr)) && \
	    (ivt_initial->entry != 0x0) && \
	    (ivt_initial->reserved1 == 0x0) && \
	    (ivt_initial->self == \
		   (uint32_t)((ulong)ivt_initial & 0xffffffff)) && \
	    (ivt_initial->csf != 0x0) && \
	    (ivt_initial->reserved2 == 0x0)) {
		/* Report boot failure if DCD pointer is found in IVT */
		if (ivt_initial->dcd != 0x0)
			puts("Error: DCD pointer must be 0\n");
		else
			return 1;
	}

	puts("Error: Invalid IVT structure\n");
	puts("\nAllowed IVT structure:\n");
	puts("IVT HDR       = 0x4X2000D1\n");
	puts("IVT ENTRY     = 0xXXXXXXXX\n");
	puts("IVT RSV1      = 0x0\n");
	puts("IVT DCD       = 0x0\n");		/* Recommended */
	puts("IVT BOOT_DATA = 0xXXXXXXXX\n");	/* Commonly 0x0 */
	puts("IVT SELF      = 0xXXXXXXXX\n");	/* = ddr_start + ivt_offset */
	puts("IVT CSF       = 0xXXXXXXXX\n");
	puts("IVT RSV2      = 0x0\n");

	/* Invalid IVT structure */
	return 0;
}

bool imx_hab_is_enabled(void)
{
	struct imx_sec_config_fuse_t *fuse =
		(struct imx_sec_config_fuse_t *)&imx_sec_config_fuse;
	uint32_t reg;
	int ret;

	ret = fuse_read(fuse->bank, fuse->word, &reg);
	if (ret) {
		puts("\nSecure boot fuse read error\n");
		return ret;
	}

	return (reg & IS_HAB_ENABLED_BIT) == IS_HAB_ENABLED_BIT;
}

int imx_hab_authenticate_image(uint32_t ddr_start, uint32_t image_size,
			       uint32_t ivt_offset)
{
	ulong load_addr = 0;
	size_t bytes;
	ulong ivt_addr = 0;
	int result = 1;
	ulong start;
	struct ivt *ivt;
	enum hab_status status;

	if (!imx_hab_is_enabled())
		puts("hab fuse not enabled\n");

	printf("\nAuthenticate image from DDR location 0x%x...\n",
	       ddr_start);

	hab_caam_clock_enable(1);

	/* Calculate IVT address header */
	ivt_addr = (ulong) (ddr_start + ivt_offset);
	ivt = (struct ivt *)ivt_addr;

	/* Verify IVT header bugging out on error */
	if (!validate_ivt(ivt))
		goto hab_authentication_exit;

	start = ddr_start;
	bytes = image_size;

	/* Verify CSF */
	if (!csf_is_valid(ivt, start, bytes))
		goto hab_authentication_exit;

	if (hab_rvt_entry() != HAB_SUCCESS) {
		puts("hab entry function fail\n");
		goto hab_exit_failure_print_status;
	}

	status = hab_rvt_check_target(HAB_TGT_MEMORY, (void *)(ulong)ddr_start, bytes);
	if (status != HAB_SUCCESS) {
		printf("HAB check target 0x%08x-0x%08zx fail\n",
		       ddr_start, ddr_start + bytes);
		goto hab_exit_failure_print_status;
	}
#ifdef DEBUG
	printf("\nivt_offset = 0x%x, ivt addr = 0x%x\n", ivt_offset, ivt_addr);
	printf("ivt entry = 0x%08x, dcd = 0x%08x, csf = 0x%08x\n", ivt->entry,
	       ivt->dcd, ivt->csf);
	puts("Dumping IVT\n");
	print_buffer(ivt_addr, (void *)(ivt_addr), 4, 0x8, 0);

	puts("Dumping CSF Header\n");
	print_buffer(ivt->csf, (void *)(ivt->csf), 4, 0x10, 0);

#if  !defined(CONFIG_SPL_BUILD)
	get_hab_status();
#endif

	puts("\nCalling authenticate_image in ROM\n");
	printf("\tivt_offset = 0x%x\n", ivt_offset);
	printf("\tstart = 0x%08lx\n", start);
	printf("\tbytes = 0x%x\n", bytes);
#endif

#ifndef CONFIG_ARM64
	/*
	 * If the MMU is enabled, we have to notify the ROM
	 * code, or it won't flush the caches when needed.
	 * This is done, by setting the "pu_irom_mmu_enabled"
	 * word to 1. You can find its address by looking in
	 * the ROM map. This is critical for
	 * authenticate_image(). If MMU is enabled, without
	 * setting this bit, authentication will fail and may
	 * crash.
	 */
	/* Check MMU enabled */
	if (is_soc_type(MXC_SOC_MX6) && get_cr() & CR_M) {
		if (is_mx6dq()) {
			/*
			 * This won't work on Rev 1.0.0 of
			 * i.MX6Q/D, since their ROM doesn't
			 * do cache flushes. don't think any
			 * exist, so we ignore them.
			 */
			if (!is_mx6dqp())
				writel(1, MX6DQ_PU_IROM_MMU_EN_VAR);
		} else if (is_mx6sdl()) {
			writel(1, MX6DLS_PU_IROM_MMU_EN_VAR);
		} else if (is_mx6sl()) {
			writel(1, MX6SL_PU_IROM_MMU_EN_VAR);
		}
	}
#endif

	load_addr = (ulong)hab_rvt_authenticate_image(
			HAB_CID_UBOOT,
			ivt_offset, (void **)&start,
			(size_t *)&bytes, NULL);
	if (hab_rvt_exit() != HAB_SUCCESS) {
		puts("hab exit function fail\n");
		load_addr = 0;
	}

hab_exit_failure_print_status:
#if !defined(CONFIG_SPL_BUILD)
	get_hab_status();
#endif

hab_authentication_exit:

	if (load_addr != 0 || !imx_hab_is_enabled())
		result = 0;

	return result;
}

int authenticate_image(uint32_t ddr_start, uint32_t raw_image_size)
{
	uint32_t ivt_offset;
	size_t bytes;

	ivt_offset = (raw_image_size + ALIGN_SIZE - 1) &
					~(ALIGN_SIZE - 1);
	bytes = ivt_offset + IVT_SIZE + CSF_PAD_SIZE;

	return imx_hab_authenticate_image(ddr_start, bytes, ivt_offset);
}
