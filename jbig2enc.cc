// Copyright 2006 Google Inc. All Rights Reserved.
// Author: agl@imperialviolet.org (Adam Langley)
//
// Copyright (C) 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>

#include <allheaders.h>
#include <pix.h>

#include <math.h>
#if defined(sun)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

#include "jbmap.h"
#include "jbvector.h"

#include "jbig2arith.h"
#include "jbig2structs.h"
#include "jbig2segments.h"

#ifdef __MINGW32__
unsigned short my_htons(unsigned short x) {
  return x >> 8 & 0xff | x << 8 & 0xff00;
}
unsigned long my_htonl(unsigned long x) {
  return x >> 24 & 0xff | x >> 8 & 0xff00 | x << 8 & 0xff0000 |
         x << 24 & 0xff000000ul;
}
#endif

// -----------------------------------------------------------------------------
// This is the context for a multi-page JBIG2 document.
// -----------------------------------------------------------------------------
struct jbig2ctx {
  struct JbClasser *classer;  // the leptonica classifier
  int xres, yres;  // ppi for the X and Y direction
  bool full_headers;  // true if we are producing a full JBIG2 file
  bool pdf_page_numbering;  // true if all text pages are page "1" (pdf mode)
  int segnum;  // current segment number
  int symtab_segment;  // the segment number of the symbol table
  // a map from page number a list of components for that page
  jbmap<int, jbvector<int> > pagecomps;
  // for each page, the list of symbols which are only used on that page
  jbmap<int, jbvector<unsigned> > single_use_symbols;
  // the number of symbols in the global symbol table
  int num_global_symbols;
  jbvector<int> page_xres, page_yres;
  jbvector<int> page_width, page_height;
  // Used to store the mapping from symbol number to the index in the global
  // symbol dictionary.
  jbmap<int, int> symmap;
  bool refinement;
  PIXA *avg_templates;  // grayed templates
  int refine_level;
  // only used when using refinement
    // the number of the first symbol of each page
    jbvector<int> baseindexes;
};

// see comments in .h file
struct jbig2ctx *
jbig2_init(float thresh, float weight, int xres, int yres, bool full_headers,
           int refine_level) {
  struct jbig2ctx *ctx = new jbig2ctx;
  ctx->xres = xres;
  ctx->yres = yres;
  ctx->full_headers = full_headers;
  ctx->pdf_page_numbering = !full_headers;
  ctx->segnum = 0;
  ctx->symtab_segment = -1;
  ctx->refinement = refine_level >= 0;
  ctx->refine_level = refine_level;
  ctx->avg_templates = NULL;

  ctx->classer = jbCorrelationInitWithoutComponents(JB_CONN_COMPS /* components == 0 */,
                                                    9999, 9999,
                                                    thresh, weight);
  return ctx;
}

// see comments in .h file
void
jbig2_destroy(struct jbig2ctx *ctx) {
  if (ctx->avg_templates) pixaDestroy(&ctx->avg_templates);
  jbClasserDestroy(&ctx->classer);
  delete ctx;
}

#define F(x) memcpy(ret + offset, &x, sizeof(x)) ; offset += sizeof(x)
#define G(x, y) memcpy(ret + offset, x, y); offset += y;
#define SEGMENT(x) x.write(ret + offset); offset += x.size();

#undef F
#undef G

// see comments in .h file
u8 *
jbig2_encode_generic(struct Pix *const bw, const bool full_headers, const int xres,
                     const int yres, const bool duplicate_line_removal,
                     int *const length) {
  int segnum = 0;

  if (!bw) return NULL;
  pixSetPadBits(bw, 0);

  struct jbig2_file_header header;
  if (full_headers) {
    memset(&header, 0, sizeof(header));
    header.n_pages = htonl(1);
    header.organisation_type = 1;
    memcpy(&header.id, JBIG2_FILE_MAGIC, 8);
  }

  // setup compression
  struct jbig2enc_ctx ctx;
  jbig2enc_init(&ctx);

  Segment seg, seg2, endseg;
  jbig2_page_info pageinfo;
  memset(&pageinfo, 0, sizeof(pageinfo));
  jbig2_generic_region genreg;
  memset(&genreg, 0, sizeof(genreg));

  seg.number = segnum;
  segnum++;
  seg.type = segment_page_information;
  seg.page = 1;
  seg.len = sizeof(struct jbig2_page_info);
  pageinfo.width = htonl(bw->w);
  pageinfo.height = htonl(bw->h);
  pageinfo.xres = htonl(xres ? xres : bw->xres);
  pageinfo.yres = htonl(yres ? yres : bw->yres);
  pageinfo.is_lossless = 1;

#ifdef SURPRISE_MAP
  dprintf(3, "P5\n%d %d 255\n", bw->w, bw->h);
#endif

  jbig2enc_bitimage(&ctx, (u8 *) bw->data, bw->w, bw->h, duplicate_line_removal);
  jbig2enc_final(&ctx);
  const int datasize = jbig2enc_datasize(&ctx);

  seg2.number = segnum;
  segnum++;
  seg2.type = segment_imm_generic_region;
  seg2.page = 1;
  seg2.len = sizeof(genreg) + datasize;

  endseg.number = segnum;
  segnum++;
  endseg.page = 1;

  genreg.width = htonl(bw->w);
  genreg.height = htonl(bw->h);
  if (duplicate_line_removal) {
    genreg.tpgdon = true;
  }
  genreg.a1x = 3;
  genreg.a1y = -1;
  genreg.a2x = -3;
  genreg.a2y = -1;
  genreg.a3x = 2;
  genreg.a3y = -2;
  genreg.a4x = -2;
  genreg.a4y = -2;

  const int totalsize = seg.size() + sizeof(pageinfo) + seg2.size() +
                        sizeof(genreg) + datasize +
                        (full_headers ? (sizeof(header) + 2*endseg.size()) : 0);
  u8 *const ret = (u8 *) malloc(totalsize);
  int offset = 0;

#define F(x) memcpy(ret + offset, &x, sizeof(x)) ; offset += sizeof(x)
  if (full_headers) {
    F(header);
  }
  SEGMENT(seg);
  F(pageinfo);
  SEGMENT(seg2);
  F(genreg);
  jbig2enc_tobuffer(&ctx, ret + offset);
  offset += datasize;

  if (full_headers) {
    endseg.type = segment_end_of_page;
    SEGMENT(endseg);
    endseg.number += 1;
    endseg.type = segment_end_of_file;
    SEGMENT(endseg);
  }

  if (totalsize != offset) abort();

  jbig2enc_dealloc(&ctx);

  *length = offset;

  return ret;
}

