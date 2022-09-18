#pragma once
#include <stdio.h>

namespace minidis {

const BYTE OP_RET = 0xc3;

typedef enum {
    MT_INVALID, // invalid instruction
    MT_RET,
    MT_NOP,
    MT_INTX,
    MT_INT3,
    MT_CALL,
    MT_JUMP,
    MT_COND_JUMP,
    MT_LOOP,
    MT_PUSH,
    MT_POP,
    MT_ARITHMETICAL,
    MT_MOV,
    MT_TEST,
    MT_OTHER,
    MT_NONE, // invalid input
    COUNT_MT
} mnem_type;

}; /* namespace minidis */
