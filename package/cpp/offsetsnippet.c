/*
** 2014 May 31
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
*/

#include "offsetsnippet.h"
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

// #define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> /* amalgamator: keep */

/* Mark a function parameter as unused, to suppress nuisance compiler
** warnings. */
#ifndef UNUSED_PARAM
#define UNUSED_PARAM(X) (void)(X)
#endif

#ifndef UNUSED_PARAM2
#define UNUSED_PARAM2(X, Y) (void)(X), (void)(Y)
#endif

const int MAX_SNIPPETS = 5;

/*
** Object used to iterate through all "coalesced phrase instances" in
** a single column of the current row. If the phrase instances in the
** column being considered do not overlap, this object simply iterates
** through them. Or, if they do overlap (share one or more tokens in
** common), each set of overlapping instances is treated as a single
** match. See documentation for the highlight() auxiliary function for
** details.
**
** Usage is:
**
**   for(rc = fts5CInstIterNext(pApi, pFts, iCol, &iter);
**      (rc==SQLITE_OK && 0==fts5CInstIterEof(&iter);
**      rc = fts5CInstIterNext(&iter)
**   ){
**     printf("instance starts at %d, ends at %d\n", iter.iStart, iter.iEnd);
**   }
**
*/
typedef struct CInstIter CInstIter;
struct CInstIter
{
    const Fts5ExtensionApi *pApi; /* API offered by current FTS version */
    Fts5Context *pFts;            /* First arg to pass to pApi functions */
    int iCol;                     /* Column to search */
    int iInst;                    /* Next phrase instance index */
    int nInst;                    /* Total number of phrase instances */

    /* Output variables */
    int iStart; /* First token in coalesced phrase instance */
    int iEnd;   /* Last token in coalesced phrase instance */
};

/*
** Advance the iterator to the next coalesced phrase instance. Return
** an SQLite error code if an error occurs, or SQLITE_OK otherwise.
*/
static int fts5CInstIterNext(CInstIter *pIter)
{
    int rc = SQLITE_OK;
    pIter->iStart = -1;
    pIter->iEnd = -1;

    while (rc == SQLITE_OK && pIter->iInst < pIter->nInst)
    {
        int ip;
        int ic;
        int io;
        rc = pIter->pApi->xInst(pIter->pFts, pIter->iInst, &ip, &ic, &io);
        if (rc == SQLITE_OK)
        {
            if (ic == pIter->iCol)
            {
                int iEnd = io - 1 + pIter->pApi->xPhraseSize(pIter->pFts, ip);
                if (pIter->iStart < 0)
                {
                    pIter->iStart = io;
                    pIter->iEnd = iEnd;
                }
                else if (io <= pIter->iEnd)
                {
                    if (iEnd > pIter->iEnd)
                        pIter->iEnd = iEnd;
                }
                else
                {
                    break;
                }
            }
            pIter->iInst++;
        }
    }

    return rc;
}

/*
** Initialize the iterator object indicated by the final parameter to
** iterate through coalesced phrase instances in column iCol.
*/
static int fts5CInstIterInit(
    const Fts5ExtensionApi *pApi,
    Fts5Context *pFts,
    int iCol,
    CInstIter *pIter)
{
    int rc;

    memset(pIter, 0, sizeof(CInstIter));
    pIter->pApi = pApi;
    pIter->pFts = pFts;
    pIter->iCol = iCol;
    rc = pApi->xInstCount(pFts, &pIter->nInst);

    if (rc == SQLITE_OK)
    {
        rc = fts5CInstIterNext(pIter);
    }

    return rc;
}

/*************************************************************************
** Start of highlight() implementation.
*/
typedef struct HighlightContext HighlightContext;
struct HighlightContext
{
    CInstIter iter;          /* Coalesced Instance Iterator */
    int iPos;                /* Current token offset in zIn[] */
    int iRangeStart;         /* First token to include */
    int iRangeEnd;           /* If non-zero, last token to include */
    const char *zOpen;       /* Opening highlight */
    const char *zClose;      /* Closing highlight */
    const char *zIn;         /* Input text */
    int nIn;                 /* Size of input text in bytes */
    int iOff;                /* Current offset within zIn[] */
    int matchStartingOffset; /* Current starting offset of first matched term within zIn[] */
    int matchEndingOffset;   /* Current ending offset of last matched term within zIn[] */

    int previousMatchStartingOffset; /* Previous snippet starting offset of first matched term within zIn[] */
    int previousMatchEndingOffset;   /* Previous snippet ending offset of last matched term within zIn[] */

    char *zOut;               /* Output value */
    char *zCurrentSnippetOut; /* Text from current snippet of the match being iterated */

    int numberOfSnippets; /* Counter for how many snippets we have retrieved for the match so far */
};

/*
** Append text to the HighlightContext output string - p->zOut. Argument
** z points to a buffer containing n bytes of text to append. If n is
** negative, everything up until the first '\0' is appended to the output.
**
** If *pRc is set to any value other than SQLITE_OK when this function is
** called, it is a no-op. If an error (i.e. an OOM condition) is encountered,
** *pRc is set to an error code before returning.
*/
static void fts5HighlightAppend(
    int *pRc,
    HighlightContext *p,
    const char *z, int n, int temp)
{
    if (*pRc == SQLITE_OK && z)
    {
        if (n < 0)
            n = (int)strlen(z);
        if (temp == 1)
        {
            p->zCurrentSnippetOut = sqlite3_mprintf("%z%.*s", p->zCurrentSnippetOut, n, z);
            if (p->zCurrentSnippetOut == 0)
            {
                *pRc = SQLITE_NOMEM;
            }
        }
        else
        {
            p->zOut = sqlite3_mprintf("%z%.*s", p->zOut, n, z);
            if (p->zOut == 0)
                *pRc = SQLITE_NOMEM;
        }
    }
}

/*
** Tokenizer callback used by implementation of highlight() function.
*/
static int fts5HighlightCb(
    void *pContext,     /* Pointer to HighlightContext object */
    int tflags,         /* Mask of FTS5_TOKEN_* flags */
    const char *pToken, /* Buffer containing token */
    int nToken,         /* Size of token in bytes */
    int iStartOff,      /* Start offset of token */
    int iEndOff         /* End offset of token */
)
{
    HighlightContext *p = (HighlightContext *)pContext;
    int rc = SQLITE_OK;
    int iPos;
    UNUSED_PARAM2(pToken, nToken);

    if (tflags & FTS5_TOKEN_COLOCATED)
        return SQLITE_OK;
    iPos = p->iPos++;

    if (p->iRangeEnd > 0)
    {
        if (iPos < p->iRangeStart)
            return SQLITE_OK;
        else if(iPos > p->iRangeEnd)
            return SQLITE_DONE;
        if (p->iRangeStart && iPos == p->iRangeStart)
            p->iOff = iStartOff;
    }
    // printf("p->zOut 1: %s\n", p->zOut);
    // printf("Range start: %d\n", p->iRangeStart);
    // printf("Range end: %d\n", p->iRangeEnd);
    // printf("iStartOff: %d\n", iStartOff);
    // printf("iEndOff: %d\n", iEndOff);
    // printf("iPos: %d\n", iPos);
    // printf("p->iter.iStart: %d\n", p->iter.iStart);
    // printf("p->iter.iEnd: %d\n", p->iter.iEnd);
    if (iPos == p->iter.iStart)
    {
        fts5HighlightAppend(&rc, p, &p->zIn[p->iOff], iStartOff - p->iOff, 1);
        // printf("p->zOut 2: %s\n", p->zOut);
        fts5HighlightAppend(&rc, p, p->zOpen, -1, 1);
        // printf("p->zOut 3: %s\n", p->zOut);
        p->iOff = iStartOff;
        if (p->matchStartingOffset == -1)
        {
            p->matchStartingOffset = iStartOff;
        }
    }

    if (iPos == p->iter.iEnd)
    {
        if (p->iRangeEnd && p->iter.iStart < p->iRangeStart)
        {
            fts5HighlightAppend(&rc, p, p->zOpen, -1, 1);
            if (p->matchStartingOffset == -1)
            {
                p->matchStartingOffset = iStartOff;
            }
            // printf("p->zOut 4: %s\n", p->zOut);
        }
        fts5HighlightAppend(&rc, p, &p->zIn[p->iOff], iEndOff - p->iOff, 1);
        // printf("p->zOut 5: %s\n", p->zOut);
        fts5HighlightAppend(&rc, p, p->zClose, -1, 1);
        // printf("p->zOut 6: %s\n", p->zOut);
        p->iOff = iEndOff;
        p->matchEndingOffset = iEndOff;
        if (rc == SQLITE_OK)
        {
            rc = fts5CInstIterNext(&p->iter);
        }
    }

    if (p->iRangeEnd > 0 && iPos == p->iRangeEnd)
    {
        fts5HighlightAppend(&rc, p, &p->zIn[p->iOff], iEndOff - p->iOff, 1);
        // printf("p->zOut 7: %s\n", p->zOut);
        p->iOff = iEndOff;
        if (iPos >= p->iter.iStart && iPos < p->iter.iEnd)
        {
            fts5HighlightAppend(&rc, p, p->zClose, -1, 1);
            p->matchEndingOffset = iEndOff;

            // printf("p->zOut 8: %s\n", p->zOut);
        }
    }
    // printf("match starting offset: %d\n", p->matchStartingOffset);
    // printf("match ending offset: %d\n", p->matchEndingOffset);
    return rc;
}

/*
** Context object passed to the fts5SentenceFinderCb() function.
*/
typedef struct Fts5SFinder Fts5SFinder;
struct Fts5SFinder
{
    int iPos;         /* Current token position */
    int nFirstAlloc;  /* Allocated size of aFirst[] */
    int nFirst;       /* Number of entries in aFirst[] */
    int *aFirst;      /* Array of first token in each sentence */
    const char *zDoc; /* Document being tokenized */
};

/*
** Add an entry to the Fts5SFinder.aFirst[] array. Grow the array if
** necessary. Return SQLITE_OK if successful, or SQLITE_NOMEM if an
** error occurs.
*/
static int fts5SentenceFinderAdd(Fts5SFinder *p, int iAdd)
{
    if (p->nFirstAlloc == p->nFirst)
    {
        int nNew = p->nFirstAlloc ? p->nFirstAlloc * 2 : 64;
        int *aNew;

        aNew = (int *)sqlite3_realloc64(p->aFirst, nNew * sizeof(int));
        if (aNew == 0)
            return SQLITE_NOMEM;
        p->aFirst = aNew;
        p->nFirstAlloc = nNew;
    }
    p->aFirst[p->nFirst++] = iAdd;
    return SQLITE_OK;
}

/*
** This function is an xTokenize() callback used by the auxiliary snippet()
** function. Its job is to identify tokens that are the first in a sentence.
** For each such token, an entry is added to the SFinder.aFirst[] array.
*/
static int fts5SentenceFinderCb(
    void *pContext,     /* Pointer to HighlightContext object */
    int tflags,         /* Mask of FTS5_TOKEN_* flags */
    const char *pToken, /* Buffer containing token */
    int nToken,         /* Size of token in bytes */
    int iStartOff,      /* Start offset of token */
    int iEndOff         /* End offset of token */
)
{
    int rc = SQLITE_OK;

    UNUSED_PARAM2(pToken, nToken);
    UNUSED_PARAM(iEndOff);

    if ((tflags & FTS5_TOKEN_COLOCATED) == 0)
    {
        Fts5SFinder *p = (Fts5SFinder *)pContext;
        if (p->iPos > 0)
        {
            int i;
            char c = 0;
            for (i = iStartOff - 1; i >= 0; i--)
            {
                c = p->zDoc[i];
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
                    break;
            }
            if (i != iStartOff - 1 && (c == '~'))
            {
                rc = fts5SentenceFinderAdd(p, p->iPos);
            }
        }
        else
        {
            rc = fts5SentenceFinderAdd(p, 0);
        }
        p->iPos++;
    }
    return rc;
}

static int fts5SnippetScore(
    const Fts5ExtensionApi *pApi, /* API offered by current FTS version */
    Fts5Context *pFts,            /* First arg to pass to pApi functions */
    int nDocsize,                 /* Size of column in tokens */
    unsigned char *aSeen,         /* Array with one element per query phrase */
    int iCol,                     /* Column to score */
    int iPos,                     /* Starting offset to score */
    int nToken,                   /* Max tokens per snippet */
    int *pnScore,                 /* OUT: Score */
    int *piPos                    /* OUT: Adjusted offset */
)
{
    int rc;
    int i;
    int ip = 0;
    int ic = 0;
    int iOff = 0;
    int iFirst = -1;
    int nInst;
    int nScore = 0;
    int iLast = 0;
    sqlite3_int64 iEnd = (sqlite3_int64)iPos + nToken;

    rc = pApi->xInstCount(pFts, &nInst);
    for (i = 0; i < nInst && rc == SQLITE_OK; i++)
    {
        rc = pApi->xInst(pFts, i, &ip, &ic, &iOff);
        if (rc == SQLITE_OK && ic == iCol && iOff >= iPos && iOff < iEnd)
        {
            nScore += (aSeen[ip] ? 1 : 1000);
            aSeen[ip] = 1;
            if (iFirst < 0)
                iFirst = iOff;
            iLast = iOff + pApi->xPhraseSize(pFts, ip);
        }
    }

    *pnScore = nScore;
    if (piPos)
    {
        sqlite3_int64 iAdj = iFirst - (nToken - (iLast - iFirst)) / 2;
        if ((iAdj + nToken) > nDocsize)
            iAdj = nDocsize - nToken;
        if (iAdj < 0)
            iAdj = 0;
        *piPos = (int)iAdj;
    }

    return rc;
}

/*
** Return the value in pVal interpreted as utf-8 text. Except, if pVal
** contains a NULL value, return a pointer to a static string zero
** bytes in length instead of a NULL pointer.
*/
static const char *fts5ValueToText(sqlite3_value *pVal)
{
    const char *zRet = (const char *)sqlite3_value_text(pVal);
    return zRet ? zRet : "";
}

/*
** Implementation of snippet() function.
*/
static void fts5SnippetFunction(
    const Fts5ExtensionApi *pApi, /* API offered by current FTS version */
    Fts5Context *pFts,            /* First arg to pass to pApi functions */
    sqlite3_context *pCtx,        /* Context for returning result/error */
    int nVal,                     /* Number of values in apVal[] array */
    sqlite3_value **apVal         /* Array of trailing arguments */
)
{
    HighlightContext ctx;
    int rc = SQLITE_OK;          /* Return code */
    int iCol;                    /* 1st argument to snippet() */
    const char *zEllips;         /* 4th argument to snippet() */
    int nToken;                  /* 5th argument to snippet() */
    int nInst = 0;               /* Number of instance matches this row */
    int i;                       /* Used to iterate through instances */
    int nPhrase;                 /* Number of phrases in query */
    unsigned char *aSeen;        /* Array of "seen instance" flags */
    int iBestCol;                /* Column containing best snippet */
    int iBestStart = 0;          /* First token of best snippet */
    int iPreviousBestStart = -1; /* We use this to keep track of any repeated snippets, due to another match belonging to it, and skip them*/
    int nBestScore = 0;          /* Score of best snippet */
    int nColSize = 0;            /* Total size of iBestCol in tokens */
    // Fts5SFinder sFinder;         /* Used to find the beginnings of sentences */
    int nCol;

    if (nVal != 5)
    {
        const char *zErr = "wrong number of arguments to function snippet()";
        sqlite3_result_error(pCtx, zErr, -1);
        return;
    }

    nCol = pApi->xColumnCount(pFts);
    memset(&ctx, 0, sizeof(HighlightContext));
    iCol = sqlite3_value_int(apVal[0]);
    ctx.zOpen = fts5ValueToText(apVal[1]);
    ctx.zClose = fts5ValueToText(apVal[2]);
    ctx.numberOfSnippets = 0;
    ctx.matchStartingOffset = -1;
    ctx.matchEndingOffset = -1;

    zEllips = fts5ValueToText(apVal[3]);
    nToken = sqlite3_value_int(apVal[4]);

    iBestCol = (iCol >= 0 ? iCol : 0);
    nPhrase = pApi->xPhraseCount(pFts);
    aSeen = (unsigned char *)sqlite3_malloc(nPhrase);
    if (aSeen == 0)
    {
        rc = SQLITE_NOMEM;
    }
    if (rc == SQLITE_OK)
    {
        rc = pApi->xInstCount(pFts, &nInst);
    }
    // printf("Column instances: %d\n", nInst);

    if (rc == SQLITE_OK)
    {
        rc = pApi->xColumnText(pFts, iBestCol, &ctx.zIn, &ctx.nIn);
    }
    if (rc == SQLITE_OK && nColSize == 0)
    {
        rc = pApi->xColumnSize(pFts, iBestCol, &nColSize);
    }

    // memset(&sFinder, 0, sizeof(Fts5SFinder));
    for (i = 0; i < nCol; i++)
    {
        if (iCol < 0 || iCol == i)
        {
            int nDoc;
            int nDocsize;
            int ii;
            // sFinder.iPos = 0;
            // sFinder.nFirst = 0;
            // rc = pApi->xColumnText(pFts, i, &sFinder.zDoc, &nDoc);
            // if (rc != SQLITE_OK)
            //     break;
            // rc = pApi->xTokenize(pFts,
            //                      sFinder.zDoc, nDoc, (void *)&sFinder, fts5SentenceFinderCb);
            // if (rc != SQLITE_OK)
            //     break;
            rc = pApi->xColumnSize(pFts, i, &nDocsize);
            // printf("Column size: %d\n", nDocsize);
            if (rc != SQLITE_OK)
                break;

            for (ii = 0; rc == SQLITE_OK && ii < nInst && ctx.numberOfSnippets < MAX_SNIPPETS; ii++)
            {
                int ip, ic, io;
                int iAdj;
                int nScore;
                int jj;

                rc = pApi->xInst(pFts, ii, &ip, &ic, &io);
                if (ic != i)
                    continue;

                // printf("Instance match index: %d\n", ii);
                // printf("Instance match phrase: %d\n", ip);
                // printf("Instance match column: %d\n", ic);
                // printf("Instance match offset: %d\n", io);

                if (io > nDocsize)
                    rc = SQLITE_CORRUPT_VTAB;
                if (rc != SQLITE_OK)
                    continue;
                memset(aSeen, 0, nPhrase);
                rc = fts5SnippetScore(pApi, pFts, nDocsize, aSeen, i,
                                      io, nToken, &nScore, &iAdj);
                // if (rc == SQLITE_OK && nScore > nBestScore)
                // {
                nBestScore = nScore;
                iBestCol = i;
                iBestStart = iAdj;
                nColSize = nDocsize;
                // }
                // printf("Best start: %d\n", iBestStart);
                // printf("Best score: %d\n", nBestScore);
                // printf("N score: %d\n", nScore);
                // printf("io: %d\n", io);
                // printf("sFinder.nFirst: %d\n", sFinder.nFirst);

                // if (rc == SQLITE_OK && sFinder.nFirst && nDocsize > nToken)
                // {
                //     for (jj = 0; jj < (sFinder.nFirst - 1); jj++)
                //     {
                //         printf("sFinder.aFirst[jj]: %d\n", sFinder.aFirst[jj]);
                //         if (sFinder.aFirst[jj + 1] > io)
                //             break;
                //     }

                //     if (sFinder.aFirst[jj] <= io)
                //     {
                //         memset(aSeen, 0, nPhrase);
                //         rc = fts5SnippetScore(pApi, pFts, nDocsize, aSeen, i,
                //                               sFinder.aFirst[jj], nToken, &nScore, 0);

                //         nScore += (sFinder.aFirst[jj] == 0 ? 120 : 100);
                //         printf("N score: %d\n", nScore);

                //         if (rc == SQLITE_OK && nScore > nBestScore)
                //         {
                //             nBestScore = nScore;
                //             iBestCol = i;
                //             iBestStart = sFinder.aFirst[jj];
                //             nColSize = nDocsize;
                //         }
                //     }
                // }

                // printf("Best start after: %d\n", iBestStart);

                // this makes sense if we have the sentence finder above, but leaving this in case we uncomment that
                // if (iPreviousBestStart == iBestStart)
                // {
                //     continue;
                // }
                // iPreviousBestStart = iBestStart;

                if (ctx.zIn)
                {
                    if (rc == SQLITE_OK)
                    {
                        rc = fts5CInstIterInit(pApi, pFts, iBestCol, &ctx.iter);
                    }

                    ctx.iPos = 0;
                    if (ctx.previousMatchStartingOffset > ctx.matchStartingOffset || ctx.previousMatchEndingOffset < ctx.matchEndingOffset)
                    {
                        ctx.previousMatchStartingOffset = ctx.matchStartingOffset;
                        ctx.previousMatchEndingOffset = ctx.matchEndingOffset;
                    }
                    ctx.zCurrentSnippetOut = NULL;
                    ctx.matchStartingOffset = -1;
                    ctx.matchEndingOffset = -1;
                    ctx.iRangeStart = iBestStart;
                    ctx.iRangeEnd = iBestStart + nToken - 1;

                    if (iBestStart > 0)
                    {
                        fts5HighlightAppend(&rc, &ctx, zEllips, -1, 1);
                    }

                    /* Advance iterator ctx.iter so that it points to the first coalesced
                    ** phrase instance at or following position iBestStart. */
                    // printf("ctx.iter.iStart outside: %d\n", ctx.iter.iStart);
                    // printf("ctx.iter.iEnd outside: %d\n", ctx.iter.iEnd);
                    // printf("ctx.iRangeStart: %d\n", ctx.iRangeStart);
                    // printf("ctx.iRangeEnd: %d\n", ctx.iRangeEnd);

                    // int matchIterations = 0;
                    while (ctx.iter.iStart >= 0 && ctx.iter.iStart < iBestStart && rc == SQLITE_OK)
                    {
                        rc = fts5CInstIterNext(&ctx.iter);
                    }

                    if (rc == SQLITE_OK)
                    {
                        rc = pApi->xTokenize(pFts, ctx.zIn, ctx.nIn, (void *)&ctx, fts5HighlightCb);
                    }

                    // we skip any snippet that is already included in the previous snippet
                    if (ctx.previousMatchStartingOffset <= ctx.matchStartingOffset && ctx.previousMatchEndingOffset >= ctx.matchEndingOffset)
                    {
                        continue;
                    }
                    else
                    {
                        fts5HighlightAppend(&rc, &ctx, ctx.zCurrentSnippetOut, -1, 0);
                    }

                    if (ctx.iRangeEnd >= (nColSize - 1))
                    {
                        fts5HighlightAppend(&rc, &ctx, &ctx.zIn[ctx.iOff], ctx.nIn - ctx.iOff, 0);
                    }
                    else
                    {
                        fts5HighlightAppend(&rc, &ctx, zEllips, -1, 0);
                    }

                    fts5HighlightAppend(&rc, &ctx, "~PP~EOS~", -1, 0);

                    // add starting offset
                    int length = snprintf(NULL, 0, "%d", ctx.matchStartingOffset);
                    char *offsetToString = malloc(length + 1);
                    snprintf(offsetToString, length + 1, "%d", ctx.matchStartingOffset);
                    fts5HighlightAppend(&rc, &ctx, offsetToString, -1, 0);

                    free(offsetToString);

                    fts5HighlightAppend(&rc, &ctx, "~PP~EOL~", -1, 0);

                    ctx.numberOfSnippets++;
                }
            }
        }
    }

    // printf("Text before: %s\n", ctx.zIn);
    // printf("Text after: %s\n", ctx.zOut);
    // printf("Offset: %d\n", ctx.iOff);
    // printf("iPos: %d\n", ctx.iPos);
    // printf("Size of input: %d\n", ctx.nIn);

    if (rc == SQLITE_OK)
    {
        sqlite3_result_text(pCtx, ctx.zOut, -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_result_error_code(pCtx, rc);
    }

    sqlite3_free(ctx.zOut);
    sqlite3_free(aSeen);
    // sqlite3_free(sFinder.aFirst);
}
/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
fts5_api *fts5_api_from_db(sqlite3 *db)
{
    fts5_api *pRet = 0;
    sqlite3_stmt *pStmt = 0;

    if (SQLITE_OK == sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0))
    {
        sqlite3_bind_pointer(pStmt, 1, (void *)&pRet, "fts5_api_ptr", NULL);
        sqlite3_step(pStmt);
    }
    sqlite3_finalize(pStmt);
    return pRet;
}

/************************************************************************/

int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);

    if (!db) return SQLITE_MISUSE;

    fts5_api *fts5api = fts5_api_from_db(db);
    if (!fts5api)
    {
        *pzErrMsg = sqlite3_mprintf("No FTS5 API available");
        return SQLITE_ERROR;
    }

    if (fts5api->iVersion < 2)
    {
        *pzErrMsg = sqlite3_mprintf("FTS5 API version too old");
        return SQLITE_ERROR;
    }

    rc = fts5api->xCreateFunction(fts5api, "snippets_with_offsets", NULL, fts5SnippetFunction, NULL);
    if (rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("Failed to create snippets_with_offsets function");
    }
    return rc;
}
