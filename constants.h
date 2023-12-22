#ifndef VM2_CONSTANTS_H
#define VM2_CONSTANTS_H

// upper bytes
const char BINOP = 0;
const char Prog_command = 1;
const char LD = 2;
const char LDA = 3;
const char ST = 4;
const char Func_command = 5;
const char PATT = 6;
const char Lcommand = 7;
const char STOP = 15;

namespace BINOP_ops {
    const char plus = 1;
    const char minus = 2;
    const char mul = 3;
    const char div = 4;
    const char mod = 5;
    const char le = 6;
    const char leq = 7;
    const char ge = 8;
    const char geq = 9;
    const char eq = 10;
    const char neq = 11;
    const char _and = 12;
    const char _or = 13;
}

namespace Prog_comms {
    const char CONST = 0;
    const char STRING = 1;
    const char SEXP = 2;
    const char STI = 3;
    const char STA = 4;
    const char JMP = 5;
    const char END = 6;
    const char RET = 7;
    const char DROP = 8;
    const char DUP = 9;
    const char SWAP = 10;
    const char ELEM = 11;
}

namespace Func_comms {
    const char CJMPz = 0;
    const char CJMPnz = 1;
    const char BEGIN = 2;
    const char cBEGIN = 3;
    const char CLOSURE = 4;
    const char CALLC = 5;
    const char CALL = 6;
    const char TAG = 7;
    const char ARRAY = 8;
    const char FAIL = 9;
    const char LINE = 10;
}

namespace PATT_comms {
    const char StrCmp = 0;
    const char String = 1;
    const char Array = 2;
    const char Sexp = 3;
    const char Boxed = 4;
    const char UnBoxed = 5;
    const char Closure = 6;
}

namespace Lcomms {
    const char Lread = 0;
    const char Lwrite = 1;
    const char Llength = 2;
    const char Lstring = 3;
    const char Larray = 4;
}

namespace LD_vars {
    const char global = 0;
    const char local = 1;
    const char arg = 2;
    const char closure = 3;
}

#endif //VM2_CONSTANTS_H
