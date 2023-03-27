#ifndef __H_TXVIEW__
#define __H_TXVIEW__

#define VERSION "0.1.0 (2023/03/27)"

#ifdef XDEV68K
#define _iocs_b_bpeek     B_BPEEK
#define _iocs_b_consol    B_CONSOL
#define _iocs_b_clr_ed    B_CLR_ED
#define _iocs_b_locate    B_LOCATE
#define _iocs_b_color     B_COLOR
#define _iocs_b_left      B_LEFT
#define _iocs_b_right     B_RIGHT
#define _iocs_b_print     B_PRINT
#define _iocs_b_era_ed    B_ERA_ED
#define _iocs_b_putmes    B_PUTMES
#define _iocs_b_keysns    B_KEYSNS
#define _iocs_b_keyinp    B_KEYINP
#define _iocs_b_sftsns    B_SFTSNS
#define _iocs_bitsns      BITSNS
#define _iocs_crtmod      CRTMOD
#define _iocs_txrascpy    TXRASCPY
#define _iocs_tpalet      TPALET
#define _iocs_os_curon    OS_CURON
#define _iocs_os_curof    OS_CUROF
#define _iocs_g_clr_on    G_CLR_ON
#define _iocs_b_wpoke     B_WPOKE

#define _dos_c_fnkmod     C_FNKMOD
#define _dos_c_cls_al     C_CLS_AL
#define _dos_c_curoff     C_CUROFF
#define _dos_c_curon      C_CURON
#define _dos_fnckeygt     FNCKEYGT
#define _dos_fnckeyst     FNCKEYST
#define _dos_malloc       MALLOC
#define _dos_mfree(x)     MFREE((uint32_t)x)
#define _dos_k_keyinp     K_KEYINP
#endif

#define CONSOLE_WIDTH  (90)
#define CONSOLE_HEIGHT (30)

#define MAX_LINES (99999)
#define OVERFLOW_MARGIN (4096)

#define WAIT_VSYNC   while(!(_iocs_b_bpeek((uint8_t*)0xE88001) & 0x10))
#define WAIT_VBLANK  while( (_iocs_b_bpeek((uint8_t*)0xE88001) & 0x10))

#define TEXT_COLOR1   (0b0111001000111101)

#define HEADER_LINE  " LINE+---------+---------+---------+---------+---------+---------+---------+---------+---------+"
#define SEARCH_FWD_LINE " SEARCH FWD[                    ] ---+"
#define SEARCH_BWD_LINE " SEARCH BWD[                    ] ---+"
#define SEARCH_MAX_LEN (20)

#endif