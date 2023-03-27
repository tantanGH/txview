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

#include "keyboard.h"
#include "fpcall.h"
#include "kanji.h"
#include "txview.h"

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
//  search prompt
//
static int32_t search_prompt(uint8_t* search_word) {

  int32_t rc = -1;

  _iocs_b_consol(768-8*26, 0, 19, 0);
  _iocs_os_curon();

  _iocs_b_locate(0, 0);
  _iocs_b_print(search_word);
  _iocs_b_era_ed();
  _iocs_b_locate(0, 0);

  for (;;) {

    int16_t char_code = _dos_k_keyinp();

    if (char_code == 13) {
      // CR
      _iocs_os_curof();  
      fpcall_set_fp_mode(0);
      rc = 0;
      break;
    } else if (char_code == 27) {
      // ESC
      _iocs_os_curof();
      fpcall_set_fp_mode(0);
      rc = -1;
      break;
    } else if (char_code == 8) {
      // BS
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      if (x == (SEARCH_MAX_LEN - 1) && strlen(search_word) == SEARCH_MAX_LEN) x++;    // for line end special case
      int16_t cur_ofs = x;
      if (x > 1 && kanji_check_sjis(search_word, x - 2) == 1) {
        memmove(search_word + x - 2, search_word + x, strlen(search_word) - x + 1);
        cur_ofs -= 2;
      } else if (x > 0) {
        memmove(search_word + x - 1, search_word + x, strlen(search_word) - x + 1);
        cur_ofs--;
      }
      _iocs_b_color(3);
      _iocs_b_locate(0, 0);
      _iocs_b_print(search_word);
      _iocs_b_era_ed();
      _iocs_b_locate(cur_ofs, 0);
    } else if (char_code == 7) {
      // DEL
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      if (kanji_check_sjis(search_word, x) == 1) {
        memmove(search_word + x, search_word + x + 2, strlen(search_word) - x);
      } else {
        memmove(search_word + x, search_word + x + 1, strlen(search_word) - x);
      }
      _iocs_b_color(3);
      _iocs_b_locate(0, 0);
      _iocs_b_print(search_word);
      _iocs_b_era_ed();
      _iocs_b_locate(x, 0);
    } else if (char_code == 19) {
      // LEFT
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      if (x > 1 && kanji_check_sjis(search_word, x - 2) == 1) {
        _iocs_b_left(2);
      } else if (x > 0) {
        _iocs_b_left(1);
      }
    } else if (char_code == 4) {
      // RIGHT
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      if (kanji_check_sjis(search_word, x) == 1) {
        if (x < (SEARCH_MAX_LEN - 1) && x < strlen(search_word) - 1) {
          _iocs_b_right(2);
        }
      } else {
        if (x < SEARCH_MAX_LEN && x < strlen(search_word)) {
          _iocs_b_right(1);
        }
      }
    } else if ((char_code >= 0x20 && char_code <= 0x7f && char_code != 0x22) ||
               (char_code >= 0xa1 && char_code <= 0xdf)) {
      // hankaku codes
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      int16_t cur_ofs = x;
      int16_t len = strlen(search_word);
      if (_iocs_b_sftsns() & 0x1000) {
        // INS mode
        if (len < (SEARCH_MAX_LEN - 1)) {
          memmove(search_word + x + 1, search_word + x, len - x + 1);
          search_word[x] = char_code;
          cur_ofs++;
        }
      } else {
        // not INS mode
        if (x < (SEARCH_MAX_LEN - 0)) {
          if (search_word[x] == '\0') {
            search_word[x+1] = '\0';
          } else if (kanji_check_sjis(search_word, x) == 1) {
            search_word[x+1] = 0x20;
          }
          search_word[x] = char_code;
          if (x < (SEARCH_MAX_LEN - 1)) {
            cur_ofs++;
          }
        }
      }
      _iocs_b_color(3);
      _iocs_b_locate(0, 0);
      _iocs_b_print(search_word);
      _iocs_b_era_ed();
      _iocs_b_locate(cur_ofs, 0);
    } else if ((char_code >= 0x81 && char_code <= 0x9f) ||
               (char_code >= 0xe0 && char_code <= 0xef)) {
      // shift-jis zenkaku 1st codes
      int16_t x = _iocs_b_locate(-1,-1) >> 16;
      int16_t cur_ofs = x;
      int16_t len = strlen(search_word); 
      int16_t char_code2 = _dos_k_keyinp();    // shift-jis 2nd code
      if (_iocs_b_sftsns() & 0x1000) {
        // INS mode
        if (len < (SEARCH_MAX_LEN - 2)) {
          memmove(search_word + x + 2, search_word + x, len - x + 1);
          search_word[x] = char_code;
          search_word[x+1] = char_code2;
          cur_ofs += 2;
        }
      } else {
        // not INS mode
        if (x < (SEARCH_MAX_LEN - 1)) {
          if (search_word[x] == '\0') {
            search_word[x+2] = '\0';
          } else if (kanji_check_sjis(search_word, x+1) == 1) {
            search_word[x+2] = 0x20;
          }
          search_word[x] = char_code;
          search_word[x+1] = char_code2;
          if (x < (SEARCH_MAX_LEN - 2)) {
            cur_ofs += 2;
          } else {
            cur_ofs += 1;
          }
        }
      }
      _iocs_b_color(3);
      _iocs_b_locate(0, 0);
      _iocs_b_print(search_word);
      _iocs_b_era_ed();
      _iocs_b_locate(cur_ofs, 0);
    }
  }            

  _iocs_b_consol(16*3, 4*4, CONSOLE_WIDTH - 1, CONSOLE_HEIGHT - 1);

  return rc;
}

//
//  search forward
//
static int32_t search_forward(uint8_t** line_ptrs, uint8_t* search_word, int32_t num_lines, int32_t line_ofs) {

  int32_t new_line_ofs = line_ofs;

  _iocs_b_putmes(1, 96 - 38, 0, 38, SEARCH_FWD_LINE);

  if (search_prompt(search_word) == 0) {
    for (int32_t line = line_ofs + 1; line < num_lines; line++) {
      if (strstr(line_ptrs[line], search_word) != NULL) {
        new_line_ofs = line;
        break;
      }
    }
  }

  _iocs_b_putmes(1, 0, 0, 96, HEADER_LINE);

  return new_line_ofs;
}

//
//  search backward
//
static int32_t search_backward(uint8_t** line_ptrs, uint8_t* search_word, int32_t num_lines, int32_t line_ofs) {

  int32_t new_line_ofs = line_ofs;

  _iocs_b_putmes(1, 96 - 38, 0, 38, SEARCH_BWD_LINE);

  if (search_prompt(search_word) == 0) {
    for (int32_t line = line_ofs - 1; line >= 0; line--) {
      if (strstr(line_ptrs[line], search_word) != NULL) {
        new_line_ofs = line;
        break;
      }
    }
  }

  _iocs_b_putmes(1, 0, 0, 96, HEADER_LINE);

  return new_line_ofs; 
}

//
//  full refresh
//
static void refresh_lines(uint8_t** line_ptrs, uint16_t* line_labels, int32_t num_lines, int32_t line_ofs) {

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
    _iocs_b_era_ed();
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

  // current text palette #1
  uint16_t tpalette1_original = _iocs_tpalet(1, -1);

  // preserve original function keys
  static uint8_t funckey_original_settings[ 712 ];
  _dos_fnckeygt(0, funckey_original_settings);

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
  line_labels = (uint16_t*)_dos_malloc(sizeof(uint16_t) * MAX_LINES);
  if ((uint32_t)line_labels >= 0x81000000) {
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
  file_data = (uint8_t*)_dos_malloc(file_size + OVERFLOW_MARGIN);
  if ((uint32_t)file_data >= 0x81000000) {
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

        // determine cutout point
        size_t cut_ofs = 0;
        do {
          if (kanji_check_sjis(line_start, cut_ofs) == 1) {   // SJIS 1st byte?
            if (cut_ofs == CONSOLE_WIDTH - 1) break;
            cut_ofs++;
          }
          cut_ofs++;
        } while (cut_ofs < CONSOLE_WIDTH);

        memmove(line_start + cut_ofs + 1, line_start + cut_ofs, file_data + data_size - (line_start + cut_ofs));

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
  line_ptrs = (uint8_t**)_dos_malloc(sizeof(uint8_t*) * num_lines);
  if ((uint32_t)line_ptrs >= 0x81000000) {
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

  // text palette
  _iocs_tpalet(1, TEXT_COLOR1);

  // clear screen
  _dos_c_fnkmod(2);
  _dos_c_cls_al();
  _dos_c_curoff();

  // customize function keys
  static uint8_t funckey_settings[ 712 ];
  memset(funckey_settings, 0, 712);
  funckey_settings[ 20 * 32 + 6 * 3 ] = '\x07';   // DEL
  funckey_settings[ 20 * 32 + 6 * 5 ] = '\x13';   // LEFT
  funckey_settings[ 20 * 32 + 6 * 6 ] = '\x04';   // RIGHT
  _dos_fnckeyst(0, funckey_settings);

  // console area
  _iocs_b_consol(16*3, 4*4, CONSOLE_WIDTH - 1, CONSOLE_HEIGHT - 1);

  // header line
  _iocs_b_putmes(1, 0, 0, 96, HEADER_LINE);

  // current top line number
  int32_t line_ofs = 0;
  refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);

  // line header
  static uint8_t line_header[7];
  line_header[6] = '\0';

  // search word
  uint8_t search_word[ SEARCH_MAX_LEN + 1 ];
  memset(search_word, 0, SEARCH_MAX_LEN + 1);

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
        _iocs_b_era_ed();
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
        sprintf(line_header, "%5d|", line_labels[ line_ofs + CONSOLE_HEIGHT - 1 ]);
        _iocs_b_locate(0, CONSOLE_HEIGHT - 1);
        _iocs_b_era_ed();
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
        // ESC, 'q'
        break;
      } else if (scan_code == KEY_SCAN_CODE_HOME || scan_code == KEY_SCAN_CODE_SHIFT_COMMA) {
        // HOME, '<'
        line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_UNDO || scan_code == KEY_SCAN_CODE_SHIFT_PERIOD) {
        // UNDO, '>'
        line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_ROLLDOWN || scan_code == KEY_SCAN_CODE_B) {
        // ROLLDOWN, 'b'
        line_ofs -= CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_ROLLUP || scan_code == KEY_SCAN_CODE_SPACE) {
        // ROLLUP, SPACE
        line_ofs += CONSOLE_HEIGHT;
        if (line_ofs > num_lines - CONSOLE_HEIGHT) line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_SLASH || scan_code == KEY_SCAN_CODE_S || scan_code == KEY_SCAN_CODE_F) {
        // '/', 's', 'f'
        line_ofs = search_forward(line_ptrs, search_word, num_lines, line_ofs);
        if (line_ofs > num_lines - CONSOLE_HEIGHT) line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      } else if (scan_code == KEY_SCAN_CODE_SHIFT_SLASH || scan_code == KEY_SCAN_CODE_SHIFT_S || scan_code == KEY_SCAN_CODE_SHIFT_F) {
        // '?', 'S', 'F'
        line_ofs = search_backward(line_ptrs, search_word, num_lines, line_ofs);
        if (line_ofs > num_lines - CONSOLE_HEIGHT) line_ofs = num_lines - CONSOLE_HEIGHT;
        if (line_ofs < 0) line_ofs = 0;
        refresh_lines(line_ptrs, line_labels, num_lines, line_ofs);
      }

    }

  }

catch:

  _iocs_b_consol(0, 0, 95, 30);
  _iocs_tpalet(1, tpalette1_original);

  _dos_fnckeyst(0, funckey_original_settings);
  _dos_c_curon();
  _dos_c_cls_al();
  _dos_c_fnkmod(funckey_mode);

exit:

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  if (line_labels != NULL) {
    _dos_mfree(line_labels);
    line_labels = NULL;
  }

  if (line_ptrs != NULL) {
    _dos_mfree(line_ptrs);
    line_ptrs = NULL;
  }

  if (file_data != NULL) {
    _dos_mfree(file_data);
    file_data = NULL;
  }

  // flush key buffer
  while (_iocs_b_keysns() != 0) {
    _iocs_b_keyinp();
  }
 
  return rc;
}