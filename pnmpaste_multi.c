/* pnmpaste.c - paste a rectangle into a PNM image
**
**
** COpyright (C) 2019 by Hiroyuki Bessho.
**
** Derived from pnmpaste.c
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include <assert.h>

#if 0
#include "pm_c_util.h"
#include "mallocvar.h"
#include "nstring.h"
#include "shhopt.h"
#include "pnm.h"
#else
#define	HAVE_BOOL
#include <stdbool.h>
#include <pnm.h>
#include <netpbm-shhopt.h>
#include "util.h"
#endif


enum boolOp {REPLACE, AND, OR, XOR /*, NAND, NOR, NXOR */ };

struct Inset {
    /* parameters from command line arguments */
    const char *filename;
    int insertCol;  /* Negative means from right edge */
    int insertRow;  /* Negative means from bottom edge */

    /* work area */
    FILE *fp;
    int format;
    unsigned int cols;
    unsigned int rows;
    xelval maxVal;
};

struct CmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    enum boolOp operation;
    struct Inset *insets;
    int nInsets;
};

struct InsetFilesInfo {
    int format;
    xelval maxVal;
    int maxCols;

    int nInsets;
    struct Inset *insets;
};

static void
parseCommandLine(int argc, const char ** argv,
                 struct CmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    unsigned int replaceOpt, andOpt, orOpt, xorOpt;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "replace",     OPT_FLAG,    NULL,
            &replaceOpt,           0);
    OPTENT3(0,   "and",         OPT_FLAG,    NULL,
            &andOpt,               0);
    OPTENT3(0,   "or",          OPT_FLAG,    NULL,
            &orOpt,                0);
    OPTENT3(0,   "xor",         OPT_FLAG,    NULL,
            &xorOpt,               0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We have parms that are negative numbers */

    pm_optParseOptions3(&argc, (char **)argv, opt, sizeof opt, 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (replaceOpt + andOpt + orOpt + xorOpt > 1)
        pm_error("You may specify only one of -replace, -and, -or, and -xor");

    cmdlineP->operation =
        replaceOpt ? REPLACE :
        andOpt     ? AND     :
        orOpt      ? OR      :
        xorOpt     ? XOR     :
        replaceOpt;
        

    if (argc-1 < 3) {
        pm_error("You must specify at least 3 arguments: \"from\" file "
                 "name, insert-at column, and insert-at row.  "
                 "You specified %u", argc-1);
    }

    if ((argc - 1) % 3 != 0) {
        pm_error("The number of arguments must be multiple of 3. "
                 "You specified %u", argc-1);
    }

    cmdlineP->nInsets = (argc-1) / 3;

    MALLOCARRAY_NOFAIL(cmdlineP->insets, cmdlineP->nInsets);

    for (int i=0; i < cmdlineP->nInsets; ++i) {

        cmdlineP->insets[i].filename = argv[i*3 + 1];
        cmdlineP->insets[i].insertCol     = atoi(argv[i*3 + 2]);
        cmdlineP->insets[i].insertRow     = atoi(argv[i*3 + 3]);
    }
}



static unsigned char
leftBits(unsigned char const x,
         unsigned int  const n){
/*----------------------------------------------------------------------------
  'x' with the leftmost (high) n bits retained and the rest cleared to zero.
-----------------------------------------------------------------------------*/
    assert(n <= 8);

    return (x >> (8-n)) << (8-n);
}



static unsigned char
rightBits(unsigned char const x,
          unsigned int  const n){
/*----------------------------------------------------------------------------
  The rightmost 'n' bits of 'x'.
-----------------------------------------------------------------------------*/
    assert(n <= 8);

    return ((unsigned char)(x << (8-n))) >> (8-n);
}



static void
insertDirect(FILE *          const ifP,
             unsigned char * const destrow,
             unsigned int    const cols,
             int             const format,
             enum boolOp     const operation,
             unsigned char * const buffer) {
/*----------------------------------------------------------------------------
   Read the next row from PBM file 'ifP' and merge it according to
   'operation' into 'destrow', flush left in packed PBM format.

   'cols' and 'format' describe the 'ifP' image.

   'buffer' is a scratch buffer for our use, at least wide enough to hold
   a packed PBM row of 'ifP'.
-----------------------------------------------------------------------------*/
    /* We use pbm_readpbmrow_packed() to read whole bytes rounded up and merge
       those into 'destrow', which means we update more than we're supposed to
       if the image is not a multiple of 8 columns.  In that case, we then fix
       up the last byte by replacing the bits from the original image that we
       messed up.
    */
    unsigned int  const colBytes  = pbm_packed_bytes(cols);
    unsigned int  const last      = colBytes - 1;
    unsigned char const origRight = destrow[last];

    if (operation == REPLACE)
        pbm_readpbmrow_packed(ifP, destrow, cols, format);
    else {
        unsigned int i;

        pbm_readpbmrow_packed(ifP, buffer, cols, format);

        for (i = 0; i < colBytes; ++i) {
            switch (operation) {
            case AND: destrow[i] |= buffer[i]; break;
            case OR : destrow[i] &= buffer[i]; break;
            case XOR: destrow[i]  = ~( destrow[i] ^ buffer[i] ) ; break;
            /*
            case NAND: destrow[i] = ~( destrow[i] | buffer[i] ) ; break;
            case NOR : destrow[i] = ~( destrow[i] & buffer[i] ) ; break;
            case NXOR: destrow[i] ^= buffer[i]  ; break;
            */
            case REPLACE: assert(false); break;
            }
        }
    }

    /* destrow[] now contains garbage in all but the cols % 8 leftmost bits of
       the last byte we touched.  Those are supposed to be unchanged from the
       input, so we restore them now.
    */
    if (cols % 8 > 0)
        destrow[last] = leftBits(destrow[last], cols % 8)
            | rightBits(origRight, 8 - cols % 8);
}



static void
insertShift(FILE *          const ifP,
            unsigned char * const destrow,
            unsigned int    const cols,
            unsigned int    const format,
            unsigned int    const offset,
            enum boolOp     const operation,
            unsigned char * const buffer) {
/*----------------------------------------------------------------------------
   Same as insertDirect(), but start merging 'offset' bits from the left
   end of 'destrow'.  'offset' is less than 8.

   buffer[] is wide enough to hold a packed PBM row of *ifP plus two
   bytes of margin.
-----------------------------------------------------------------------------*/
    unsigned int  const shiftByteCt = pbm_packed_bytes(cols + offset);
    unsigned int  const last        = shiftByteCt - 1;
    unsigned char const origLeft    = destrow[0];
    unsigned char const origRight   = destrow[last];

    unsigned int const padOffset = (cols + offset) % 8;

    unsigned int i;

    assert(offset < 8);

    pbm_readpbmrow_packed(ifP, &buffer[1], cols, format);

    /* Note that buffer[0] is undefined. */

    for (i = 0; i < shiftByteCt; ++i) {
        unsigned int  const rsh = offset;
        unsigned int  const lsh = 8-rsh;
        unsigned char const t = buffer[i] << lsh | buffer[i+1] >> rsh;

        switch (operation) {
        case REPLACE: destrow[i] = t; break;
        case AND:     destrow[i] |= t; break;
        case OR :     destrow[i] &= t; break;
        case XOR:     destrow[i] = ~ (destrow[i] ^ t); break;
        /*
        case NAND:    destrow[i] = ~ (destrow[i] | t); break;
        case NOR :    destrow[i] = ~ (destrow[i] & t); break;
        case NXOR:    destrow[i] ^= t; break;
        */
        }
    }

    /* destrow[] now contains garbage in the 'offset' leftmost bits and
       8-offset rightmost bits of the last byte we touched.  Those are
       supposed to be unchanged from the input, so we restore them now.
    */

    destrow[0] = leftBits(origLeft, offset) |
        rightBits(destrow[0], 8-offset);
   
    if (padOffset % 8 > 0)
        destrow[last] = leftBits(destrow[last], padOffset) |
            rightBits(origRight , 8-padOffset);
}



static void
pastePbm(FILE *       const fpBase,
         unsigned int const baseCols,
         unsigned int const baseRows,
	 int          const baseFormat,
	 enum boolOp  const operation,
	 const struct InsetFilesInfo *insetsInfo)
{
/*----------------------------------------------------------------------------
  Fast paste for PBM
-----------------------------------------------------------------------------*/
    unsigned char * const baserow       = pbm_allocrow_packed(baseCols);
    unsigned char * const buffer        = pbm_allocrow_packed(insetsInfo->maxCols+16);
    unsigned int    const baseColByteCt = pbm_packed_bytes(baseCols);

    unsigned int row;

    pbm_writepbminit(stdout, baseCols, baseRows, 0);


    for (row = 0; row < baseRows; ++row) {
        pbm_readpbmrow_packed(fpBase, baserow, baseCols, baseFormat);
        
	

	for (int idx = 0; idx < insetsInfo->nInsets; ++idx) {
	    struct Inset *inset = &insetsInfo->insets[idx];
	    unsigned int    const shiftByteCt   = inset->insertCol / 8;
	    unsigned int    const shiftOffset   = inset->insertCol % 8;

	    if (row >= inset->insertRow && row < inset->insertRow + inset->rows) {
		if (shiftOffset == 0)
		    insertDirect(inset->fp, &baserow[shiftByteCt], inset->cols,
				 inset->format,
				 operation, buffer);
		else
		    insertShift(inset->fp, &baserow[shiftByteCt], inset->cols,
				inset->format, shiftOffset, operation, buffer);
	    }
	}

        if (baseCols % 8 > 0)
            baserow[baseColByteCt-1]
                = leftBits(baserow[baseColByteCt-1] , baseCols % 8);

        pbm_writepbmrow_packed(stdout, baserow, baseCols, 0);
    }
    pbm_freerow_packed(buffer);
    pbm_freerow_packed(baserow);
}



static void
pasteNonPbm(FILE *       const fpBase,
	    unsigned int const colsBase,
	    unsigned int const rowsBase,
            int          const formatBase,
	    xelval       const maxvalBase,
	    const struct InsetFilesInfo * const insetsInfo)
{
    /* Logic works for PBM, but cannot do bitwise operations */             

    xel * const xelrowInset = pnm_allocrow(insetsInfo->maxCols);
    xel * const xelrowBase  = pnm_allocrow(colsBase);

    unsigned int row;

    pnm_writepnminit(stdout, colsBase, rowsBase, insetsInfo->maxVal, insetsInfo->format, 0);

    for (row = 0; row < rowsBase; ++row) {
        pnm_readpnmrow(fpBase, xelrowBase, colsBase, maxvalBase, formatBase);
        pnm_promoteformatrow(xelrowBase, colsBase, maxvalBase, formatBase,
                             insetsInfo->maxVal, insetsInfo->format);

	for (int idx = 0; idx < insetsInfo->nInsets; ++idx) {
	    struct Inset *inset = &insetsInfo->insets[idx];
	    
	    if (row >= inset->insertRow && row < inset->insertRow + inset->rows) {
		unsigned int colInset;

		pnm_readpnmrow(inset->fp, xelrowInset, inset->cols, inset->maxVal,
			       inset->format);
		pnm_promoteformatrow(xelrowInset, inset->cols, inset->maxVal,
				     inset->format, insetsInfo->maxVal, insetsInfo->format);
		for (colInset = 0; colInset < inset->cols; ++colInset)
		    xelrowBase[inset->insertCol + colInset] = xelrowInset[colInset];
	    }
	}

	pnm_writepnmrow(stdout, xelrowBase, colsBase, insetsInfo->maxVal, insetsInfo->format, 0);
    }
    
    pnm_freerow(xelrowBase);
    pnm_freerow(xelrowInset);
}



static void
checkInsetFiles(int formatBase, xelval maxvalBase,
		int rowsBase, int colsBase, struct InsetFilesInfo *info)
{
    int rowsInset, colsInset;
    FILE * fpInset;

    info->format = formatBase;
    info->maxVal = maxvalBase;
    info->maxCols = 0;


    for (int idx = 0; idx < info->nInsets; ++idx) {
	unsigned int insertRow, insertCol;

	struct Inset *inset = &info->insets[idx];

	inset->fp = pm_openr(inset->filename);

	pnm_readpnminit(inset->fp, &colsInset, &rowsInset,
			&inset->maxVal, &inset->format);

	if (colsBase < colsInset)
	    pm_error(
		"Image to paste is wider than base image by %u cols",
		colsInset - colsBase);
	else if (inset->insertCol <= -colsBase)
	    pm_error(
		"x is too negative -- the second image has only %u cols",
		colsBase);
	else if (inset->insertCol >= colsBase)
	    pm_error(
		"x is too large -- the second image has only %u cols",
		colsBase);

	if (rowsBase < rowsInset)
	    pm_error(
		"Image to paste is taller than base image by %u rows",
		rowsInset - rowsBase);
	else if (inset->insertRow <= -rowsBase)
	    pm_error(
		"y is too negative -- the second image has only %u rows",
		rowsBase);
	else if (inset->insertRow >= rowsBase)
	    pm_error(
		"y is too large -- the second image has only %d rows",
		rowsBase);

	insertCol = inset->insertCol < 0 ?
	    colsBase + inset->insertCol : inset->insertCol;
	insertRow = inset->insertRow < 0 ?
	    rowsBase + inset->insertRow : inset->insertRow;

	if (insertCol + colsInset > colsBase)
		pm_error("Extends over right edge by %u pixels",
			 (insertCol + colsInset) - colsBase);
	if (insertRow + rowsInset > rowsBase)
		pm_error("Extends over bottom edge by %u pixels",
			 (insertRow + rowsInset) - rowsBase);

	info->format = MAX(PNM_FORMAT_TYPE(inset->format),
			   PNM_FORMAT_TYPE(info->format));
	info->maxVal = MAX(inset->maxVal, info->maxVal);
	info->maxCols = MAX(colsInset, info->maxCols);

	inset->rows = rowsInset;
	inset->cols = colsInset;

    }
}

int
main(int argc, const char ** argv) {

    struct CmdlineInfo cmdline;
    FILE * fpBase;
    xelval maxvalBase;
    int rowsInset, colsInset;
    int rowsBase, colsBase;
    int formatBase;
    unsigned int insertColsMax;
    struct InsetFilesInfo insetsInfo;


    pm_proginit(&argc, (char **)argv);

    parseCommandLine(argc, argv, &cmdline);

    fpBase  = pm_openr("-");
    pnm_readpnminit(fpBase, &colsBase, &rowsBase, &maxvalBase, &formatBase);

    insetsInfo.insets = cmdline.insets;
    insetsInfo.nInsets = cmdline.nInsets;
    checkInsetFiles(formatBase, maxvalBase, rowsBase, colsBase, &insetsInfo);

    if (cmdline.operation != REPLACE && insetsInfo.format != PBM_TYPE)
	pm_error("no logical operations allowed for a non-PBM image");

    if (insetsInfo.format == PBM_TYPE)
	pastePbm(fpBase, colsBase, rowsBase, formatBase, cmdline.operation, &insetsInfo);
    else
	pasteNonPbm(fpBase, colsBase, rowsBase, formatBase, maxvalBase, &insetsInfo);


    pm_close(fpBase);
    pm_close(stdout);

    return 0;
}
