#include <stdint.h>
#include "kanji.h"

//
//  utility function to check SJIS 
//
//   0 ... not sjis
//   1 ... sjis 1st char
//   2 ... sjis 2nd char
//
int32_t kanji_check_sjis(const uint8_t* str, int16_t pos) {

  // need to scan from the beginning of the string
  int16_t ofs = 0;
  int16_t sjis = 0;
  while (ofs <= pos) {
    uint8_t c = str[ ofs++ ];
    if (c == 0) {
      break;
    } else if (sjis == 1) {
      sjis = 2;
    } else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xef)) {
      sjis = 1;
    } else {
      sjis = 0;
    }
  }
  return sjis;
}
