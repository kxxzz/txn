#include "exp_a.h"






static void EXP_seqBracketChs(EXP_NodeType type, char ch[2])
{
    switch (type)
    {
    case EXP_NodeType_SeqNaked:
        break;
    case EXP_NodeType_SeqRound:
        ch[0] = '(';
        ch[1] = ')';
        break;
    case EXP_NodeType_SeqSquare:
        ch[0] = '[';
        ch[1] = ']';
        break;
    case EXP_NodeType_SeqCurly:
        ch[0] = '{';
        ch[1] = '}';
        break;
    default:
        assert(false);
        break;
    }
}








static u32 EXP_printSlTok(const EXP_Space* space, char* buf, u32 bufSize, const EXP_SpaceSrcInfo* srcInfo, EXP_Node src)
{
    assert(EXP_isTok(space, src));

    EXP_NodeInfo* info = space->nodes.data + src.id;
    const char* str = upool_elmData(space->dataPool, info->offset);
    u32 sreLen = info->length;
    u32 n;
    bool isQuotStr = false;
    if (srcInfo && (src.id < srcInfo->nodes.length))
    {
        isQuotStr = srcInfo->nodes.data[src.id].isQuotStr;
    }
    else
    {
        for (u32 i = 0; i < sreLen; ++i)
        {
            if (strchr("()[]{}\"' \t\n\r\b\f", str[i]))
            {
                isQuotStr = true;
                break;
            }
        }
    }
    if (isQuotStr)
    {
        u32 l = 2;
        for (u32 i = 0; i < sreLen; ++i)
        {
            if (' ' >= str[i])
            {
                ++l;
            }
            else if (strchr("()[]{}\"'", str[i]))
            {
                ++l;
            }
            ++l;
        }
        if (buf && (l < bufSize))
        {
            n = 0;
            buf[n++] = '"';
            for (u32 i = 0; i < sreLen; ++i)
            {
                if (' ' >= str[i])
                {
                    buf[n++] = '\\';
                }
                else if (strchr("()[]{}\"'", str[i]))
                {
                    buf[n++] = '\\';
                }
                buf[n++] = str[i];
            }
            buf[n++] = '"';
            assert(n == l);
        }
        else
        {
            n = l;
        }
    }
    else
    {
        n = snprintf(buf, bufSize, "%s", str);
    }
    return n;
}







typedef struct EXP_PrintSlSeqLevel
{
    EXP_Node src;
    u32 p;
    char ch[2];
} EXP_PrintSlSeqLevel;

typedef vec_t(EXP_PrintSlSeqLevel) EXP_PrintSlSeqStack;



static u32 EXP_printSlSeq(const EXP_Space* space, char* buf, u32 bufSize, const EXP_SpaceSrcInfo* srcInfo, EXP_Node src)
{
    EXP_PrintSlSeqStack _seqStack = { 0 };
    EXP_PrintSlSeqStack* seqStack = &_seqStack;

    assert(EXP_isSeq(space, src));
    EXP_PrintSlSeqLevel root = { src };
    vec_push(seqStack, root);

    u32 bufRemain = bufSize;
    char* bufPtr = buf;
    u32 n = 0;

    EXP_PrintSlSeqLevel* top = NULL;
    EXP_Node node;
    EXP_NodeInfo* seqInfo = NULL;
    u32 p;
next:
    if (!seqStack->length)
    {
        vec_free(seqStack);
        return n;
    }
    top = &vec_last(seqStack);
    node = top->src;
    seqInfo = space->nodes.data + node.id;
    assert(seqInfo->type > EXP_NodeType_Tok);
    p = top->p++;

    if (0 == p)
    {
        EXP_seqBracketChs(seqInfo->type, top->ch);
        if (top->ch[0])
        {
            if (1 < bufRemain)
            {
                *bufPtr = top->ch[0];
                bufRemain -= 1;
                bufPtr += 1;
            }
            else
            {
                bufRemain = 0;
                bufPtr = NULL;
            }
            n += 1;
        }
    }
    else
    {
        if (p < seqInfo->length)
        {
            if (1 < bufRemain)
            {
                *bufPtr = ' ';
                bufRemain -= 1;
                bufPtr += 1;
            }
            else
            {
                bufRemain = 0;
                bufPtr = NULL;
            }
            n += 1;
        }
        else
        {
            assert(p == seqInfo->length);
            if (top->ch[0])
            {
                assert(top->ch[1]);
                if (1 < bufRemain)
                {
                    *bufPtr = top->ch[1];
                    *(bufPtr + 1) = 0;
                    bufRemain -= 1;
                    bufPtr += 1;
                }
                else
                {
                    bufRemain = 0;
                    bufPtr = NULL;
                }
                n += 1;
            }
            vec_pop(seqStack);
            goto next;
        }
    }
    EXP_Node e = ((EXP_Node*)upool_elmData(space->dataPool, seqInfo->offset))[p];
    if (EXP_isTok(space, e))
    {
        u32 a = EXP_printSlTok(space, bufPtr, bufRemain, srcInfo, e);
        if (a < bufRemain)
        {
            bufRemain -= a;
            bufPtr += a;
        }
        else
        {
            bufRemain = 0;
            bufPtr = NULL;
        }
        n += a;
    }
    else
    {
        EXP_PrintSlSeqLevel l = { e };
        vec_push(seqStack, l);
    }
    goto next;
}








u32 EXP_printSL(const EXP_Space* space, EXP_Node node, char* buf, u32 bufSize, const EXP_SpaceSrcInfo* srcInfo)
{
    if (EXP_isTok(space, node))
    {
        return EXP_printSlTok(space, buf, bufSize, srcInfo, node);
    }
    else
    {
        return EXP_printSlSeq(space, buf, bufSize, srcInfo, node);
    }
}













typedef struct EXP_PrintMlSeqLevel
{
    EXP_Node src;
    u32 p;
    char ch[2];
} EXP_PrintMlSeqLevel;

typedef vec_t(EXP_PrintMlSeqLevel) EXP_PrintMlSeqStack;




typedef struct EXP_PrintMlContext
{
    const EXP_Space* space;
    const EXP_PrintMlOpt* opt;
    const u32 bufSize;
    char* const buf;

    u32 n;
    u32 column;
    u32 depth;

    EXP_PrintMlSeqStack seqStack;
} EXP_PrintMlContext;


static void EXP_printMlContextFree(EXP_PrintMlContext* ctx)
{
    vec_free(&ctx->seqStack);
}





static bool EXP_printMlForward(EXP_PrintMlContext* ctx, u32 a)
{
    ctx->n += a;
    ctx->column += a;
    return ctx->column <= ctx->opt->width;
}

static void EXP_printMlBack(EXP_PrintMlContext* ctx, u32 a)
{
    assert(ctx->n >= a);
    assert(ctx->column >= a);
    ctx->n -= a;
    ctx->column -= a;
}


static void EXP_printMlAddCh(EXP_PrintMlContext* ctx, char c)
{
    assert(c);
    u32 bufRemain = (ctx->bufSize > ctx->n) ? (ctx->bufSize - ctx->n) : 0;
    if (bufRemain > 0)
    {
        assert(ctx->buf);
        u32 wn = min(bufRemain - 1, 1);
        if (wn > 0)
        {
            char* bufPtr = ctx->buf + ctx->n;
            bufPtr[0] = c;
            bufPtr[1] = 0;
        }
    }
    ctx->n += 1;
    if (c != '\n')
    {
        ctx->column += 1;
    }
    else
    {
        ctx->column = 0;
    }
}


static void EXP_printMlAdd(EXP_PrintMlContext* ctx, const char* s)
{
    u32 a = (u32)strlen(s);
    u32 bufRemain = (ctx->bufSize > ctx->n) ? (ctx->bufSize - ctx->n) : 0;
    if (bufRemain > 0)
    {
        assert(ctx->buf);
        u32 wn = min(bufRemain - 1, a);
        if (wn > 0)
        {
            char* bufPtr = ctx->buf + ctx->n;
            stzncpy(bufPtr, s, wn + 1);
            bufPtr[wn] = 0;
        }
    }
    ctx->n += a;
    u32 ca = 0;
    for (u32 i = 0; i < a; ++i)
    {
        if ('\n' == s[a - 1 - i])
        {
            ctx->column = 0;
            break;
        }
        else
        {
            ++ca;
        }
    }
    ctx->column += ca;
}



static void EXP_printMlAddIdent(EXP_PrintMlContext* ctx)
{
    u32 n = ctx->opt->indent * ctx->depth;
    for (u32 i = 0; i < n; ++i)
    {
        EXP_printMlAdd(ctx, " ");
    }
}










static void EXP_printMlSeq(EXP_PrintMlContext* ctx, EXP_Node src)
{
    const EXP_Space* space = ctx->space;
    EXP_PrintMlSeqStack* seqStack = &ctx->seqStack;
    assert(!seqStack->length);

    assert(EXP_isSeq(space, src));
    EXP_PrintMlSeqLevel root = { src };
    vec_push(seqStack, root);

    EXP_PrintMlSeqLevel* top;
    EXP_NodeInfo* seqInfo;
    u32 p;
next:
    if (0 == seqStack->length)
    {
        return;
    }
    top = &vec_last(seqStack);
    seqInfo = space->nodes.data + top->src.id;
    assert(seqInfo->type > EXP_NodeType_Tok);
    p = top->p++;

    if (0 == p)
    {
        u32 bufRemain = (ctx->bufSize > ctx->n) ? (ctx->bufSize - ctx->n) : 0;
        char* bufPtr = ctx->buf ? (ctx->buf + ctx->n) : NULL;
        u32 a = EXP_printSL(space, top->src, bufPtr, bufRemain, ctx->opt->srcInfo);
        bool ok = EXP_printMlForward(ctx, a);
        if (ok)
        {
            vec_pop(seqStack);
            goto next;
        }

        EXP_printMlBack(ctx, a);

        EXP_seqBracketChs(seqInfo->type, top->ch);
        if (seqInfo->type != EXP_NodeType_SeqNaked)
        {
            assert(top->ch[0]);
            assert(top->ch[1]);
            EXP_printMlAddCh(ctx, top->ch[0]);
            if (seqInfo->type != EXP_NodeType_SeqRound)
            {
                EXP_printMlAddCh(ctx, '\n');
            }
            ++ctx->depth;
        }
    }
    else
    {
        if (p < seqInfo->length)
        {
            EXP_printMlAddCh(ctx, '\n');
        }
        else
        {
            assert(p == seqInfo->length);
            if (EXP_NodeType_SeqRound == seqInfo->type)
            {
                EXP_printMlAddCh(ctx, top->ch[1]);
            }
            else
            {
                EXP_printMlAddCh(ctx, '\n');
            }

            if (seqInfo->type != EXP_NodeType_SeqNaked)
            {
                --ctx->depth;
                if (seqInfo->type != EXP_NodeType_SeqRound)
                {
                    EXP_printMlAddIdent(ctx);
                    EXP_printMlAddCh(ctx, top->ch[1]);
                }
            }
            vec_pop(seqStack);
            goto next;
        }
    }

    if ((p > 0) || (seqInfo->type != EXP_NodeType_SeqRound))
    {
        EXP_printMlAddIdent(ctx);
    }
    EXP_Node e = ((EXP_Node*)upool_elmData(space->dataPool, seqInfo->offset))[p];
    EXP_NodeInfo* eInfo = space->nodes.data + e.id;
    switch (eInfo->type)
    {
    case EXP_NodeType_Tok:
    {
        u32 bufRemain = (ctx->bufSize > ctx->n) ? (ctx->bufSize - ctx->n) : 0;
        char* bufPtr = ctx->buf ? (ctx->buf + ctx->n) : NULL;
        u32 a = EXP_printSL(space, e, bufPtr, bufRemain, ctx->opt->srcInfo);
        EXP_printMlForward(ctx, a);
        break;
    }
    default:
    {
        EXP_PrintMlSeqLevel l = { e };
        vec_push(seqStack, l);
        break;
    }
    }
    goto next;
}





u32 EXP_printML(const EXP_Space* space, EXP_Node node, char* buf, u32 bufSize, const EXP_PrintMlOpt* opt)
{
    EXP_NodeInfo* info = space->nodes.data + node.id;
    switch (info->type)
    {
    case EXP_NodeType_Tok:
    {
        return EXP_printSL(space, node, buf, bufSize, opt->srcInfo);
    }
    default:
    {
        EXP_PrintMlContext ctx =
        {
            space, opt, bufSize, buf,
        };
        EXP_printMlSeq(&ctx, node);
        EXP_printMlContextFree(&ctx);
        return ctx.n;
    }
    }
}
















































































































