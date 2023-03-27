#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef XDEV68K
#include <iocslib.h>
#include <doslib.h>
#else
#include <x68k/iocs.h>
#include <x68k/dos.h>
#endif

#include "himem.h"
#include "keyboard.h"

#ifdef XDEV68K
#define _iocs_b_bpeek     B_BPEEK
#define _iocs_b_consol    B_CONSOL
#define _iocs_b_clr_ed    B_CLR_ED
#define _iocs_b_locate    B_LOCATE
#define _iocs_b_color     B_COLOR
#define _iocs_b_print     B_PRINT
#define _iocs_b_era_ed    B_ERA_ED
#define _iocs_b_putmes    B_PUTMES
#define _iocs_b_keysns    B_KEYSNS
#define _iocs_b_keyinp    B_KEYINP
#define _iocs_b_sftsns    B_SFTSNS
#define _iocs_bitsns      BITSNS
#define _iocs_crtmod      CRTMOD
#define _iocs_txrascpy    TXRASCPY

#define _dos_c_fnkmod     C_CNKMOD
#define _dos_c_cls_al     C_CLS_AL
#define _dos_c_curoff     C_CUROFF
#define _dos_c_curon      C_CURON
#endif

#define VERSION "0.1.0 (2023/03/27)"

#define CONSOLE_WIDTH  (90)
#define CONSOLE_HEIGHT (30)

#define MAX_LINES (99999)
#define OVERFLOW_MARGIN (4096)

#define WAIT_VSYNC   while(!(_iocs_b_bpeek((uint8_t*)0xE88001) & 0x10))
#define WAIT_VBLANK  while( (_iocs_b_bpeek((uint8_t*)0xE88001) & 0x10))

//
//  show help message
//
static void show_help_message() {
  printf("TXVIEW.X - A simple text viewer for X680x0 " VERSION " by tantan\n");
  printf("usage: txview [options] <text-file>\n");
  printf("options:\n");
  printf("     -s ... slow smooth scroll\n");
  printf("     -h ... show version and help message\n");
  printf("\n");
  printf("key bindings:\n");
  printf("  ESC/Q       ... quit viwer\n");
  printf("  LEFT/RIGHT  ... scroll (normal)\n");
  printf("  UP/DOWN     ... scroll (fast)\n");
  printf("  ROLLUP/DOWN ... page up/down\n");
  printf("  HOME/UNDO   ... jump to top/end\n");
}

//
//  full refresh
//
static void refresh(uint8_t** line_ptrs, uint16_t* line_labels, int32_t num_lines, int32_t line_ofs) {

  static uint8_t line_header[7];
  line_header[6] = '\0';

  WAIT_VSYNC;
  WAIT_VBLANK;

  for (int16_t i = 0; i < CONSOLE_HEIGHT; i++) {
    if (line_ofs + i >= num_lines) {
      _iocs_b_clr_ed();
      break;
    }
    sprintf(line_header, "%5d|", line_labels[ line_ofs + i ]);
    _iocs_b_locate(0, i);
    _iocs_b_era_al();
    _iocs_b_putmes(1, 0, 1 + i, 5, line_header);
    _iocs_b_color(3);
    _iocs_b_print(line_ptrs[ line_ofs + i ]);
  }
}

//
//  main
//
int32_t main(int32_t argc, uint8_t* argv[]) {

  // default return code
  int32_t rc = -1;

  // scroll speed   (1:normal 0:slow)
  int16_t scroll_speed = 1;

  // source file name
  uint8_t* file_name = NULL;

  // source buffer pointer
  uint8_t* file_data = NULL;

  // source file pointer
  FILE* fp = NULL;

  // line pointers
  uint8_t** line_ptrs = NULL;

  // line number label pointers
  uint16_t* line_labels = NULL;

  // number of total lines
  int32_t num_lines = 0;

  // current function key mode
  int16_t funckey_mode = _dos_c_fnkmod(-1);

  // check command line
  if (argc < 2) {
    show_help_message();
    goto exit;
  }

  // parse command lines
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else if (argv[i][1] == 's') {
        scroll_speed = 0;
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (file_name != NULL) {
        printf("error: only 1 file can be specified.\n");
        goto exit;
      }
      file_name = argv[i];
    }
  }

  if (file_name == NULL) {
    show_help_message();
    goto exit;
  }

  // line label buffers
  line_labels = (uint16_t*)himem_malloc(sizeof(uint16_t) * MAX_LINES, 0);
  if (line_labels == NULL) {
    printf("error: out of memory.\n");
    goto exit;    
  }

  // read text file data into memory buffer as binary
  fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("error: failed to open the source file (%s).\n", file_name);
    goto exit;
  }

  // check file size
  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // allocate buffer with some margin for line wraps
  file_data = (uint8_t*)himem_malloc(file_size + OVERFLOW_MARGIN, 0);
  if (file_data == NULL) {
    printf("error: out of memory.\n");
    goto exit;
  }

  // read data
  size_t read_len = 0;
  do {
    size_t len = fread(file_data + read_len, 1, file_size - read_len, fp);
    if (len == 0) break;
    read_len += len;
  } while (read_len < file_size);
  fclose(fp);
  fp = NULL;

  // add LF if last line does not have it
  if (file_data[file_size - 1] != '\n' && file_data[file_size - 1] != '\x1a') {
    file_data[file_size] = '\n';
  } else {
    file_data[file_size] = '\0';
  }

  // preprocess lines
  size_t data_size = file_size + 1;
  uint8_t* line_start = file_data;
  uint16_t line_label = 1;
  int32_t num_wraps = 0;
  size_t ofs = 0;
  do {
    uint8_t c = file_data[ofs];
    if (c == '\n' || c == '\x1a') {
      file_data[ofs] = '\0';
      while (strlen(line_start) > CONSOLE_WIDTH) {
        memmove(line_start + CONSOLE_WIDTH + 1, line_start + CONSOLE_WIDTH, file_data + data_size - (line_start + CONSOLE_WIDTH));
        ofs++;
        data_size++;
        line_start[CONSOLE_WIDTH] = '\0';
        line_labels[num_lines] = line_label;
        num_lines++;
        //printf("%d,%d,%d,%d\n",num_lines,strlen(line_start),ofs,data_size);
        line_start += CONSOLE_WIDTH + 1;
        if (++num_wraps >= OVERFLOW_MARGIN) {
          printf("error: too many line wraps.\n");
          goto exit;
        }
      }
      line_labels[num_lines] = line_label;
      num_lines++;
      if (num_lines == MAX_LINES) break;
      //printf("%d,%d,%d,%d\n",num_lines,strlen(line_start),ofs,data_size);
      line_start = file_data + ofs + 1;
      line_label++;
    }
    ofs++;
  } while (ofs < data_size);

  // line pointer buffers
  line_ptrs = (uint8_t**)himem_malloc(sizeof(uint8_t*) * num_lines, 0);
  if (line_ptrs == NULL) {
    printf("error: out of memory.\n");
    goto exit;
  }

  // fill buffers
  line_ptrs[0] = file_data;
  for (int32_t i = 1; i < num_lines; i++) {
    line_ptrs[i] = line_ptrs[ i - 1 ] + strlen(line_ptrs[ i - 1 ]) + 1;
  }

  // init screen mode
  _iocs_crtmod(16);

  // clear screen
  _dos_c_fnkmod(2);
  _dos_c_cls_al();
  _dos_c_curoff();

  // console area
  _iocs_b_consol(16*3, 4*4, CONSOLE_WIDTH - 1, CONSOLE_HEIGHT - 1);

  // header line
  _iocs_b_putmes(1, 0, 0, 96, " LINE+---------+---------+---------+---------+---------+---------+---------+---------+---------+");

  // current top line number
  int32_t line_ofs = 0;
  refresh(line_ptrs, line_labels, num_lines, line_ofs);

  static uint8_t line_header[7];
  line_header[6] = '\0';

  // main loop
  for (;;) {

    // keyboard scan without buffer
    int32_t sense_code7 = _iocs_bitsns(7);
    int32_t sense_code4 = _iocs_bitsns(4);
    if (sense_code7 & KEY_SNS_LEFT) {
      // LEFT
      if (line_ofs > 0) {
        WAIT_VSYNC;
        if (scroll_speed == 0) {
          for (int16_t i = 0; i < 4; i++) {
            WAIT_VBLANK;
            _iocs_txrascpy(122 * 256 + 123, CONSOLE_HEIGHT * 16 / 4 - 1, 3 + 0x8000);
          }
        } else {
          for (int16_t i = 0; i < 2; i++) {
            WAIT_VBLANK;
            _iocs_txrascpy(121 * 256 + 123, CONSOLE_HEIGHT * 16 / 4 - 2, 3 + 0x8000);
          }          
        }
        line_ofs--;
        sprintf(line_header, "%5d|", line_labels[ line_ofs ]);
        _iocs_b_locate(0, 0);
        _iocs_b_era_ed();
        _iocs_b_putmes(1, 0, 1, 5, line_header);
        _iocs_b_color(3);
        _iocs_b_print(line_ptrs[ line_ofs ]);

      }
    } else if (sense_code7 & KEY_SNS_RIGHT) {
      // RIGHT
      if (num_lines > line_ofs + CONSOLE_HEIGHT) {
        while(!(_iocs_b_bpeek((uint8_t*)0xE88001) & 0x10));
        if (scroll_speed == 0) {
          for (int16_t i = 0; i < 4; i++) {
            WAIT_VBLANK;
            _iocs_txrascpy(5 * 256 + 4, CONSOLE_HEIGHT * 16 / 4 - 1, 3 + 0x0000);
          }
        } else {
          for (int16_t i = 0; i < 2; i++) {
            WAIT_VBLANK;
            _iocs_txrascpy(6 * 256 + 4, CONSOLE_HEIGHT * 16 / 4 - 2, 3 + 0x0000);
          }          
        }
        line_ofs++;
        sprintf(line_header, "%5d|", line_labels[ line_ofs + CONSOLE_HEIGHT - 1 ]);
        _iocs_b_locate(0, CONSOLE_HEIGHT - 1);
        _iocs_b_era_al();
        _iocs_b_putmes(1, 0, 1 + CONSOLE_HEIGHT - 1, 5, line_header);
        _iocs_b_color(3);
        _iocs_b_print(line_ptrs[ line_ofs + CONSOLE_HEIGHT - 1 ]);

      }
    } else if (sense_code7 & KEY_SNS_UP || sense_code4 & KEY_SNS_K) {
      // UP, k
      if (line_ofs > 0) {
        WAIT_VSYNC;
        for (int16_t i = 0; i < 1; i++) {
          WAIT_VBLANK;
          _iocs_txrascpy(119 * 256 + 123, CONSOLE_HEIGHT * 16 / 4 - 4, 3 + 0x8000);
        }
        line_ofs--;
        sprintf(line_header, "%5d|", line_labels[ line_ofs ]);
        _iocs_b_locate(0, 0);
        _iocs_b_era_ed();
        _iocs_b_putmes(1, 0, 1, 5, line_header);
        _iocs_b_color(3);
        _iocs_b_print(line_ptrs[ line_ofs ]);
      }
    } else if (sense_code7 == KEY_SNS_DOWN || sense_code4 & KEY_SNS_J) {
      // DOWN, j
      if (num_lines > line_ofs + CONSOLE_HEIGHT) {
        WAIT_VSYNC;
        for (int16_t i = 0; i < 1; i++) {
          WAIT_VBLANK;
          _iocs_txrascpy(8 * 256 + 4, CONSOLE_HEIGHT * 16 / 4 - 4, 3 + 0x0000);
        }
        line_ofs++;
        sprintf(line_header, "%4d|", line_labels[ line_ofs + CONSOLE_HEIGHT - 1 ]);
        _iocs_b_locate(0, CONSOLE_HEIGHT - 1);
        _iocs_b_era_al();
        _iocs_b_putmes(1, 0, 1 + CONSOLE_HEIGHT - 1, 5, line_header);
        _iocs_b_color(3);
        _iocs_b_print(line_ptrs[ line_ofs + CONSOLE_HEIGHT - 1 ]);
      }
    }

    // keyboard scan with advanced key input buffer
    if (_iocs_b_keysns() != 0) {
      
      int16_t scan_code = _iocs_b_keyinp() >> 8;
      int16_t shift_sense = _iocs_b_sftsns();

      if (shift_sense & 0x01) {
        scan_code += 0x100;
      }
      if (shift_sense & 0x02) {
        scan_code += 0x200;
      }

      if (scan_code == KEY_SCAN_CODE_ESC || scan_code == KEY_SCAN_CODE_Q) {
        // ESC, Q
        break;
      } else if (scan_code == KEY_SCAN_CODE_HOME || scan_code == KEY_SCAN_CODE_SHIFT_COMMA) {
        // HOME, <
        line_ofs = 0;
        refresh(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_UNDO || scan_code == KEY_SCAN_CODE_SHIFT_PERIOD) {
        // UNDO, >
        line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_ROLLDOWN || scan_code == KEY_SCAN_CODE_B) {
        // ROLLDOWN, b
        line_ofs -= CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_ROLLUP || scan_code == KEY_SCAN_CODE_SPACE) {
        // ROLLUP, SPACE
        line_ofs += CONSOLE_HEIGHT;
        if (line_ofs > num_lines - CONSOLE_HEIGHT) line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh(line_ptrs, line_labels, num_lines, line_ofs);
      }

    }

  }

catch:

  _iocs_b_consol(0, 0, 95, 30);

  _dos_c_curon();
  _dos_c_cls_al();
  _dos_c_fnkmod(funckey_mode);

exit:

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  if (line_labels != NULL) {
    himem_free(line_labels, 0);
    line_labels = NULL;
  }

  if (line_ptrs != NULL) {
    himem_free(line_ptrs, 0);
    line_ptrs = NULL;
  }

  if (file_data != NULL) {
    himem_free(file_data, 0);
    file_data = NULL;
  }

  // flush key buffer
  while (_iocs_b_keysns() != 0) {
    _iocs_b_keyinp();
  }
 
  return rc;
}