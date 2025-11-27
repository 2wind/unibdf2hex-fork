/**
   @file unibdf2hex.c

   @brief unibdf2hex - Convert a BDF file into a unifont.hex file

   @author Paul Hardy, January 2008

   @copyright Copyright (C) 2008, 2013 Paul Hardy

   Note: currently this has hard-coded code points for glyphs extracted
   from Wen Quan Yi to create the Unifont source file "wqy.hex".
*/
/*
   LICENSE:

      This program is free software: you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation, either version 2 of the License, or
      (at your option) any later version.
   
      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
      GNU General Public License for more details.
   
      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNISTART 0x0000	///< First Unicode code point to examine
#define UNISTOP 0xFFFF	///< Last Unicode code point to examine

#define MAXBUF 256	///< Maximum allowable input file line length - 1

/**
   @brief The main function.

   @return Exit status is always 0 (successful termination).
*/
int
main (void)
{
   int i;
   int digitsout;  /* how many hex digits we output in a bitmap */
   int thispoint;
   char inbuf[MAXBUF];
   int bbxx, bbxy, bbxxoff, bbxyoff;

   int descent=4; /* font descent wrt baseline */
   int startrow;  /* row to start glyph        */
   unsigned rowout;

   int GLYPH_HEIGHT = 16;  ///< 원하는 글리프 높이 (M)
   int GLYPH_WIDTH  =  8;  ///< 원하는 글리프 폭 (N)

   // --- 수정 사항 시작: BDF 헤더에서 폭과 높이 읽어오기 ---
   int dwidth_x = 0;
   int font_height = 0;

   // BDF 헤더 부분을 읽어 필요한 정보를 추출합니다.
   while (fgets (inbuf, MAXBUF - 1, stdin) != NULL) {
     // DWIDTH (기본 폭) 찾기
     if (strncmp (inbuf, "DWIDTH ", 7) == 0) {
       sscanf (&inbuf[7], "%d", &dwidth_x);
     }
     // FONTBOUNDINGBOX (높이) 찾기
     else if (strncmp (inbuf, "FONTBOUNDINGBOX ", 16) == 0) {
       int temp_w, temp_h;
       sscanf (&inbuf[16], "%d %d", &temp_w, &temp_h);
       font_height = temp_h;
     }
     // STARTCHAR을 만나면 헤더가 끝났다고 간주하고 루프를 빠져나옵니다.
     else if (strncmp (inbuf, "STARTCHAR", 9) == 0) {
       break;
     }
   }

   // 읽어온 정보를 전역 변수에 적용합니다.
   if (dwidth_x > 0) {
     GLYPH_WIDTH = dwidth_x; // BDF DWIDTH를 기본 폭 N으로 설정
   }
   if (font_height > 0) {
     GLYPH_HEIGHT = font_height; // BDF 높이를 기본 높이 M으로 설정
   }

   fprintf(stderr, "INFO: Glyph Height (M) set to %d\n", GLYPH_HEIGHT);
   fprintf(stderr, "INFO: Normal Glyph Width (N) set to %d\n", GLYPH_WIDTH);

   int current_width; // 현재 처리할 글리프의 출력 폭 (8 또는 16)
   int current_height = GLYPH_HEIGHT; // 현재 처리할 글리프의 출력 높이 (16)
   int hex_chars_per_row; // 한 행당 출력해야 할 16진수 자리수 (2 또는 4)

   while (fgets (inbuf, MAXBUF - 1, stdin) != NULL) {
      if (strncmp (inbuf, "ENCODING ", 9) == 0) {
         sscanf (&inbuf[9], "%d", &thispoint); /* get code point */
         /*
            If we want this code point, get the BBX (bounding box) and
            BITMAP information.
         */
         if ((thispoint >= 0x1100 && thispoint <= 0x11FF) || // Hangul Jamo
             (thispoint >= 0xA960 && thispoint <= 0xA97F) || // Hangul Jamo
             (thispoint >= 0xD7B0 && thispoint <= 0xD7FF) || // Hangul Jamo
             (thispoint >= 0x2E80 && thispoint <= 0x2EFF) || // CJK Radicals Supplement
             (thispoint >= 0x2F00 && thispoint <= 0x2FDF) || // Kangxi Radicals
             (thispoint >= 0x2FF0 && thispoint <= 0x2FFF) || // Ideographic Description Characters
             (thispoint >= 0x3001 && thispoint <= 0x303F) || // CJK Symbols and Punctuation (U+3000 is a space)
             (thispoint >= 0x3100 && thispoint <= 0x312F) || // Bopomofo
             (thispoint >= 0x31A0 && thispoint <= 0x31BF) || // Bopomofo extend
             (thispoint >= 0x31C0 && thispoint <= 0x31EF) || // CJK Strokes
             (thispoint >= 0x3400 && thispoint <= 0x4DBF) || // CJK Unified Ideographs Extension A
             (thispoint >= 0x4E00 && thispoint <= 0x9FCF) || // CJK Unified Ideographs
             (thispoint >= 0xAC00 && thispoint <= 0xD7A3) || // Hangul Syllable
             (thispoint >= 0xF900 && thispoint <= 0xFAFF))   // CJK Compatibility Ideographs
         {
              current_width = GLYPH_WIDTH * 2; // 16
         }
         else if ((thispoint >= UNISTART && thispoint <= UNISTOP)){
              current_width = GLYPH_WIDTH;  //
         }
         else continue;

         while (fgets (inbuf, MAXBUF - 1, stdin) != NULL &&
           strncmp (inbuf, "BBX ", 4) != 0); /* find bounding box */

         sscanf (&inbuf[4], "%d %d %d %d", &bbxx, &bbxy, &bbxxoff, &bbxyoff);
         while (fgets (inbuf, MAXBUF - 1, stdin) != NULL &&
           strncmp (inbuf, "BITMAP", 6) != 0); /* find bitmap start */

         hex_chars_per_row = current_width / 4;
         fprintf (stdout, "%04X:", thispoint);
         digitsout = 0;

         /* Print initial blank rows */
         startrow = descent + bbxyoff + bbxy;

         /* Force everything to 16 pixels wide */
         for (i = current_height; i > startrow; i--) {
           // current_width에 맞춰 00 또는 0000 출력
           fprintf (stdout, "%*.*s", hex_chars_per_row, hex_chars_per_row, "0000");
           digitsout += hex_chars_per_row;
         }
         // 6. 비트맵 데이터 복사 및 폭 조정
         while (fgets (inbuf, MAXBUF - 1, stdin) != NULL &&
           strncmp (inbuf, "END", 3) != 0) {
           sscanf (inbuf, "%X", &rowout);

            // BDF의 bbxx 폭을 무시하고, 단순히 current_width로 제한하고 오프셋만 적용
            // rowout은 보통 BDF 파일에 따라 8비트, 16비트 등으로 정의되어 있음.

            // 왼쪽 오프셋 적용
            rowout >>= bbxxoff;

            // current_width에 맞춰 출력 포맷 결정 (%02X 또는 %04X)
            fprintf (stdout, "%0*X", hex_chars_per_row, rowout);
            digitsout += hex_chars_per_row;
           }

           // 7. 하단 빈 행(Padding) 출력
           int expected_digits = current_height * hex_chars_per_row;
           while (digitsout < expected_digits) {
             // current_width에 맞춰 00 또는 0000 출력
             fprintf (stdout, "%*.*s", hex_chars_per_row, hex_chars_per_row, "0000");
             digitsout += hex_chars_per_row;
           }
           fprintf (stdout,"\n");
      }
   }
   exit (0);
}
