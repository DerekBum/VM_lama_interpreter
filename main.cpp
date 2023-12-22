#include <iostream>
#include "constants.h"

extern "C" {
    #include "runtime.h"

    extern void __init (void);
    extern int Llength(void*);
    extern void* Lstring (void *p);
    extern void* Bstring(void*);
    extern void* Bclosure_with_data (int bn, void *entry, int* data_inp);
    extern void* Barray_with_data (int bn, int* data_inp);
    extern void* Bsexp_with_data (int bn, int* data_, int tag);
}

static inline int Lread () {
    int result = BOX(0);

    printf ("> ");
    fflush (stdout);
    int ret = scanf  ("%d", &result);

    if (ret != 1) {
        failure((char*)"Failed to scanf");
    }

    return BOX(result);
}

static inline int Lwrite (int n) {
    printf ("%d\n", UNBOX(n));
    fflush (stdout);

    return 0;
}

static inline void* Belem (void *p, int i) {
    data *a = (data *)BOX(NULL);

    ASSERT_BOXED(".elem:1", p);
    ASSERT_UNBOXED(".elem:2", i);

    a = TO_DATA(p);
    i = UNBOX(i);

    if (TAG(a->tag) == STRING_TAG) {
        return (void*) BOX(a->contents[i]);
    }

    return (void*) ((int*) a->contents)[i];
}

static inline char* de_hash (int n) {
    //  static char *chars = (char*) BOX (NULL);
    static char buf[6] = {0,0,0,0,0,0};
    char *p = (char *) BOX (NULL);
    p = &buf[5];

    *p-- = 0;

    while (n != 0) {
        *p-- = chars [n & 0x003F];
        n = n >> 6;
    }

    return ++p;
}

static inline int LtagHash (char *s) {
    char *p;
    int  h = 0, limit = 0;

    p = s;

    while (*p && limit++ <= 4) {
        char *q = chars;
        int pos = 0;

        for (; *q && *q != *p; q++, pos++);

        if (*q) h = (h << 6) | pos;
        else failure ((char*)"tagHash: character not found: %c\n", *p);

        p++;
    }

    if (strncmp (s, de_hash (h), 5) != 0) {
        failure ((char*)"%s <-> %s\n", s, de_hash(h));
    }

    return BOX(h);
}

static inline int Btag (void *d, int t, int n) {
    data *r;

    if (UNBOXED(d)) return BOX(0);
    else {
        r = TO_DATA(d);
        return BOX(TAG(r->tag) == SEXP_TAG && TO_SEXP(d)->tag == UNBOX(t) && LEN(r->tag) == UNBOX(n));
    }
}

static inline int Barray_patt (void *d, int n) {
    data *r;

    if (UNBOXED(d)) return BOX(0);
    else {
        r = TO_DATA(d);
        return BOX(TAG(r->tag) == ARRAY_TAG && LEN(r->tag) == UNBOX(n));
    }
}

static inline int Bstring_patt (void *x, void *y) {
    data *rx = (data *) BOX (NULL),
            *ry = (data *) BOX (NULL);

    ASSERT_STRING(".string_patt:2", y);

    if (UNBOXED(x)) return BOX(0);
    else {
        rx = TO_DATA(x); ry = TO_DATA(y);

        if (TAG(rx->tag) != STRING_TAG) return BOX(0);

        return BOX(strcmp (rx->contents, ry->contents) == 0 ? 1 : 0);
    }
}

static inline int Bstring_tag_patt (void *x) {
    if (UNBOXED(x)) return BOX(0);

    return BOX(TAG(TO_DATA(x)->tag) == STRING_TAG);
}

static inline int Barray_tag_patt (void *x) {
    if (UNBOXED(x)) return BOX(0);

    return BOX(TAG(TO_DATA(x)->tag) == ARRAY_TAG);
}

static inline int Bsexp_tag_patt (void *x) {
    if (UNBOXED(x)) return BOX(0);

    return BOX(TAG(TO_DATA(x)->tag) == SEXP_TAG);
}

static inline int Bboxed_patt (void *x) {
    return BOX(UNBOXED(x) ? 0 : 1);
}

static inline int Bunboxed_patt (void *x) {
    return BOX(UNBOXED(x) ? 1 : 0);
}

static inline int Bclosure_tag_patt (void *x) {
    if (UNBOXED(x)) return BOX(0);

    return BOX(TAG(TO_DATA(x)->tag) == CLOSURE_TAG);
}

static inline void* Belem_unboxed (void *p, int i) {
    data *a = (data *)BOX(NULL);

    ASSERT_BOXED(".elem:1", p);
    ASSERT_UNBOXED(".elem:2", i);

    a = TO_DATA(p);
    i = UNBOX(i);

    if (TAG(a->tag) == STRING_TAG) {
        return (void*) (a->contents + i);
    }

    return ((int*) a->contents) + i;
}

static inline void* Bsta (void *v, int i, void *x) {
    if (UNBOXED(i)) {
        ASSERT_BOXED(".sta:3", x);

        if (TAG(TO_DATA(x)->tag) == STRING_TAG)((char*) x)[UNBOX(i)] = (char) UNBOX(v);
        else ((int*) x)[UNBOX(i)] = (int) v;

        return v;
    }

    * (void**) i = v;

    return v;
}

extern int32_t *__gc_stack_top, *__gc_stack_bottom;

class file_loader {
public:
    char* string_table_ptr;
    int* global_ptr;
    char* bytecode_ptr;
private:
    char* content;

public:
    file_loader(const char* file) {
        FILE* f = fopen(file, "rb");

        int string_table_size, global_size, public_table_size;

        int read_size = fread(&string_table_size, sizeof(int), 1, f);
        if (read_size != 1) {
            failure((char*)"Cannot read header");
        }
        read_size = fread(&global_size, sizeof(int), 1, f);
        if (read_size != 1) {
            failure((char*)"Cannot read header");
        }
        read_size = fread(&public_table_size, sizeof(int), 1, f);
        if (read_size != 1) {
            failure((char*)"Cannot read header");
        }
        global_ptr = new int[global_size];

        fseek(f, 0, SEEK_END);
        auto content_size = ftell(f);
        content_size -= 3 * sizeof(int32_t);
        content = new char[content_size];

        fseek(f, 3 * sizeof(int32_t), SEEK_SET);
        read_size = fread(content, 1, content_size, f);
        if (read_size != content_size) {
            failure((char*)"Cannot read content");
        }
        fclose(f);

        string_table_ptr = content + public_table_size * 2 * sizeof(int);
        bytecode_ptr = string_table_ptr + string_table_size;
    }
    ~file_loader() {
        delete[] content;
        delete[] global_ptr;
    }
};

static int32_t* STACK_HEAD;
static int MAX_STACK_SIZE = 8192 * 2;

class lama_c_interpreter {
public:
    file_loader* file;

    char* bytecode;
public:
    lama_c_interpreter(file_loader* file) : file(file), bytecode(file->bytecode_ptr) {
        // stack grows down
        STACK_HEAD = (new int[MAX_STACK_SIZE]) + MAX_STACK_SIZE;
        __gc_stack_top = STACK_HEAD;
        __gc_stack_bottom = STACK_HEAD;
    }

    ~lama_c_interpreter() {
        // delete allocated stack (it grows down)
        delete[] (__gc_stack_bottom - MAX_STACK_SIZE);
    }

    inline void push(int32_t val) {
        if (__gc_stack_bottom - MAX_STACK_SIZE == __gc_stack_top) {
            failure((char*)"Illegal push -- too many elements in the stack");
        }
        // stack grows down
        __gc_stack_top--;
        *(__gc_stack_top) = val;
    }
    inline int32_t pop() {
        if (__gc_stack_top >= STACK_HEAD) {
            failure((char*)"Illegal pop -- no elements in the stack");
        }
        // stack grows down
        return *(__gc_stack_top++);
    }
    inline int32_t get_int() {
        int32_t val = *reinterpret_cast<int32_t*>(bytecode);
        bytecode += sizeof(int32_t);
        return val;
    }
    inline char* get_string() {
        int32_t var = get_int();
        return &file->string_table_ptr[var];
    }
    int32_t* get_lds(char lower_bytes, int32_t id) {
        switch (lower_bytes) {
            case LD_vars::global:
                return file->global_ptr + id;
            case LD_vars::local:
                return STACK_HEAD - id - 1;
            case LD_vars::arg:
                return STACK_HEAD + id + 3;
            case LD_vars::closure: {
                auto n = *(STACK_HEAD + 1);
                auto arg = STACK_HEAD + n + 2;
                int32_t* closure = reinterpret_cast<int32_t*>(*arg);
                auto cl_to = Belem_unboxed(closure, BOX(id + 1));
                return reinterpret_cast<int32_t*>(cl_to);
            }
            default:
                // error
                failure((char*)"incorrect LD operation");
                return nullptr;
        }
    }
};

#define DEFINE_BINOPS(def)\
    def(BINOP_ops::plus, +)\
    def(BINOP_ops::minus, -)\
    def(BINOP_ops::mul, *) \
    def(BINOP_ops::div, /)\
    def(BINOP_ops::mod, %)\
    def(BINOP_ops::le, <)\
    def(BINOP_ops::leq, <=)\
    def(BINOP_ops::ge, >)\
    def(BINOP_ops::geq, >=)\
    def(BINOP_ops::eq, ==)\
    def(BINOP_ops::neq, !=)\
    def(BINOP_ops::_and, &&)\
    def(BINOP_ops::_or, ||)

#define DEFINE_PATT(def)\
    def(PATT_comms::String, Bstring_tag_patt)\
    def(PATT_comms::Array, Barray_tag_patt)\
    def(PATT_comms::Sexp, Bsexp_tag_patt) \
    def(PATT_comms::Boxed, Bboxed_patt)\
    def(PATT_comms::UnBoxed, Bunboxed_patt)\
    def(PATT_comms::Closure, Bclosure_tag_patt)

int main(int argc, char* argv[]) {
    __init();

    if (argc != 2) {
        failure((char*)"wrong number of arguments, expected: 1\n");
    }
    file_loader loader = file_loader(argv[1]);

    lama_c_interpreter interp = lama_c_interpreter(&loader);

    // run interpreter
    do {
        char current = *interp.bytecode++;
        char upper_bytes = current / 16;
        char lower_bytes = current % 16;
        switch (upper_bytes) {
            case STOP:
                return 0;
            case BINOP: {
                auto right = UNBOX(interp.pop());
                auto left = UNBOX(interp.pop());
                switch (lower_bytes) {
                    #define IMPLEMENT_BINOP(opcode, op) case opcode: interp.push(BOX(left op right)); break;
                        DEFINE_BINOPS(IMPLEMENT_BINOP)
                    #undef IMPLEMENT_BINOP
                    default:
                        // error
                        failure((char *) "incorrect BINOP operation %d", lower_bytes);
                }
                break;
            }
            case Prog_command:
                switch (lower_bytes) {
                    case Prog_comms::CONST:
                        interp.push(BOX(interp.get_int()));
                        break;
                    case Prog_comms::STRING: {
                        auto val = interp.get_int();
                        auto str = reinterpret_cast<int32_t>(Bstring(interp.file->string_table_ptr + val));
                        interp.push(str);
                        break;
                    }
                    case Prog_comms::SEXP: {
                        char *str = interp.get_string();
                        auto len = interp.get_int(), tag = LtagHash(str);

                        int32_t *last = __gc_stack_top, *first = last + len - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto res = reinterpret_cast<int32_t>(Bsexp_with_data(BOX(len + 1), __gc_stack_top, tag));
                        __gc_stack_top += len;
                        interp.push(res);
                        break;
                    }
                    case Prog_comms::STI:
                        // No impl
                        break;
                    case Prog_comms::STA: {
                        auto val = reinterpret_cast<void*>(interp.pop());
                        auto ind = interp.pop();
                        void* to;
                        if (!UNBOXED(ind)) {
                            to = Bsta(val, ind, nullptr);
                        } else {
                            auto ds = reinterpret_cast<void*>(interp.pop());
                            to = Bsta(val, ind, ds);
                        }
                        interp.push(reinterpret_cast<int32_t>(to));
                        break;
                    }
                    case Prog_comms::JMP: {
                        auto len = interp.get_int();
                        interp.bytecode = interp.file->bytecode_ptr + len;
                        break;
                    }
                    case Prog_comms::END: {
                        auto rest = interp.pop();
                        __gc_stack_top = STACK_HEAD;
                        STACK_HEAD = reinterpret_cast<int32_t*>(*(__gc_stack_top++));
                        if (__gc_stack_top == STACK_HEAD) {
                            interp.bytecode = nullptr;
                            break;
                        }
                        auto n = interp.pop();
                        char* ext = reinterpret_cast<char*>(interp.pop());
                        __gc_stack_top += n;
                        interp.push(rest);
                        interp.bytecode = ext;
                        break;
                    }
                    case Prog_comms::RET:
                        // No impl
                        break;
                    case Prog_comms::DROP:
                        interp.pop();
                        break;
                    case Prog_comms::DUP: {
                        auto val = interp.pop();
                        interp.push(val);
                        interp.push(val);
                        break;
                    }
                    case Prog_comms::SWAP: {
                        auto a = interp.pop(), b = interp.pop();
                        interp.push(b); interp.push(a);
                        break;
                    }
                    case Prog_comms::ELEM: {
                        auto val = interp.pop();
                        void* pt = reinterpret_cast<void*>(interp.pop());
                        auto res = Belem(pt, val);
                        interp.push(reinterpret_cast<int32_t>(res));
                        break;
                    }
                    default:
                        // error
                        failure((char*)"incorrect operation");
                }
                break;
            case LD: {
                auto id = interp.get_int();
                int32_t* val = interp.get_lds(lower_bytes, id);
                interp.push(*val);
                break;
            }
            case LDA: {
                auto id = interp.get_int();
                int32_t* val = interp.get_lds(lower_bytes, id);
                interp.push(reinterpret_cast<int32_t>(val));
                break;
            }
            case ST: {
                auto id = interp.get_int();
                auto val = interp.pop();
                *interp.get_lds(lower_bytes, id) = val;
                interp.push(val);
                break;
            }
            case Func_command:
                switch (lower_bytes) {
                    case Func_comms::CJMPz: {
                        auto len = interp.get_int();
                        auto check = interp.pop();
                        if (UNBOX(check) == 0) {
                            interp.bytecode = interp.file->bytecode_ptr + len;
                        }
                        break;
                    }
                    case Func_comms::CJMPnz: {
                        auto len = interp.get_int();
                        auto check = interp.pop();
                        if (UNBOX(check) != 0) {
                            interp.bytecode = interp.file->bytecode_ptr + len;
                        }
                        break;
                    }
                    case Func_comms::BEGIN: {
                        interp.get_int();
                        auto n = interp.get_int();
                        interp.push(reinterpret_cast<int32_t>(STACK_HEAD));
                        STACK_HEAD = __gc_stack_top;
                        for (int i = 0; i < n; i++) {
                            interp.push(BOX(0));
                        }
                        break;
                    }
                    case Func_comms::cBEGIN: {
                        interp.get_int();
                        auto n = interp.get_int();
                        interp.push(reinterpret_cast<int32_t>(STACK_HEAD));
                        STACK_HEAD = __gc_stack_top;
                        for (int i = 0; i < n; i++) {
                            interp.push(BOX(0));
                        }
                        break;
                    }
                    case Func_comms::CLOSURE: {
                        auto len = interp.get_int(), n = interp.get_int();
                        if (n > 2048) {
                            failure((char*)"Too long binds count for CLOSURE operation");
                        }
                        int32_t bind[2048];
                        for (int i = 0; i < n; i++) {
                            char c = *interp.bytecode;
                            interp.bytecode++;
                            auto val = interp.get_int();
                            bind[i] = *interp.get_lds(c, val);
                        }
                        auto closure = Bclosure_with_data(BOX(n), interp.file->bytecode_ptr + len, bind);
                        interp.push(reinterpret_cast<int32_t>(closure));
                        break;
                    }
                    case Func_comms::CALLC: {
                        auto n = interp.get_int();
                        auto el = reinterpret_cast<int32_t*>(__gc_stack_top[n]);
                        auto elem = Belem(el, BOX(0));
                        char* callc = reinterpret_cast<char*>(elem);
                        int32_t *last = __gc_stack_top, *first = last + n - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto bcode = reinterpret_cast<int32_t>(interp.bytecode);
                        interp.push(bcode);
                        interp.push(n + 1);
                        interp.bytecode = callc;
                        break;
                    }
                    case Func_comms::CALL: {
                        auto len = interp.get_int(), n = interp.get_int();
                        int32_t *last = __gc_stack_top, *first = last + n - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto bcode = reinterpret_cast<int32_t>(interp.bytecode);
                        interp.push(bcode);
                        interp.push(n);
                        interp.bytecode = interp.file->bytecode_ptr + len;
                        break;
                    }
                    case Func_comms::TAG: {
                        auto str = interp.get_string();
                        auto n = interp.get_int();
                        auto hash = LtagHash(str);
                        auto store = interp.pop();
                        void* data = reinterpret_cast<void*>(store);
                        auto tag = Btag(data, hash, BOX(n));
                        interp.push(tag);
                        break;
                    }
                    case Func_comms::ARRAY: {
                        auto n = interp.get_int();
                        int32_t* pt = reinterpret_cast<int32_t*>(interp.pop());
                        auto arr = Barray_patt(pt, BOX(n));
                        interp.push(arr);
                        break;
                    }
                    case Func_comms::FAIL: {
                        failure((char*)"Failed with FAIL");
                        break;
                    }
                    case Func_comms::LINE:
                        interp.get_int();
                        break;
                    default:
                        // error
                        failure((char*)"incorrect operation");
                }
                break;
            case PATT: {
                int32_t* patt = reinterpret_cast<int32_t*>(interp.pop());
                switch (lower_bytes) {
                    case PATT_comms::StrCmp: {
                        int32_t* pt = reinterpret_cast<int32_t*>(interp.pop());
                        interp.push(Bstring_patt(patt, pt));
                        break;
                    }
                    #define IMPLEMENT_PATT(opcode, ext_func) case opcode: interp.push(ext_func(patt)); break;
                        DEFINE_PATT(IMPLEMENT_PATT)
                    #undef IMPLEMENT_PATT
                    default:
                        // error
                        failure((char*)"incorrect PATT operation");
                }
                break;
            }
            case Lcommand:
                switch (lower_bytes) {
                    case Lcomms::Lread:
                        interp.push(Lread());
                        break;
                    case Lcomms::Lwrite: {
                        auto val = interp.pop();
                        interp.push(Lwrite(val));
                        break;
                    }
                    case Lcomms::Llength: {
                        void* val = reinterpret_cast<void*>(interp.pop());
                        interp.push(Llength(val));
                        break;
                    }
                    case Lcomms::Lstring: {
                        void* val = reinterpret_cast<void*>(interp.pop());
                        auto str = Lstring(val);
                        auto cast = reinterpret_cast<int32_t>(str);
                        interp.push(cast);
                        break;
                    }
                    case Lcomms::Larray: {
                        auto n = interp.get_int();
                        int32_t *last = __gc_stack_top, *first = last + n - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto arr = Barray_with_data(BOX(n), __gc_stack_top);
                        __gc_stack_top += n;
                        interp.push(reinterpret_cast<int32_t>(arr));
                        break;
                    }
                    default:
                        // error
                        failure((char*)"incorrect operation");
                }
                break;
            default:
                // error
                failure((char*)"incorrect operation %d");
        }
    } while (interp.bytecode != nullptr);
    return 0;
}
