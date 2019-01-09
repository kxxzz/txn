#include "a.h"



typedef enum PRIM_NodeType
{
    PRIM_NodeType_Val,
    PRIM_NodeType_Exp,
} PRIM_NodeType;


typedef struct PRIM_Node
{
    PRIM_NodeType type;
    union
    {
        u32 val;
        u32 exp;
    };
} PRIM_Node;


typedef vec_t(PRIM_Node) PRIM_Exp;
typedef vec_t(PRIM_Exp) PRIM_ExpVec;
typedef vec_t(u32) PRIM_ExpIdVec;


typedef struct PRIM_Space
{
    PRIM_ExpVec exps;
    PRIM_ExpIdVec idles;
} PRIM_Space;


PRIM_Space PRIM_newSpace(void)
{
    PRIM_Space space = { 0 };
    return space;
}


void PRIM_spaceFree(PRIM_Space* space)
{
    vec_free(&space->idles);
    for (u32 i = 0; i < space->exps.length; ++i)
    {
        vec_free(space->exps.data + i);
    }
    vec_free(&space->exps);
}
















































































