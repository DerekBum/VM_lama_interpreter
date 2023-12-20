#include <iostream>
#include "constants.h"

extern "C" {
    #include "runtime.h"

    extern void __init (void);
    extern int Lread();
    extern int Lwrite(int);
    extern int Llength(void*);
    extern void* Lstring (void *p);
    extern void* Bstring(void*);
    extern void* Belem (void *p, int i);
    extern int LtagHash (char *s);
    extern int Btag (void *d, int t, int n);
    extern int Barray_patt (void *d, int n);
    extern int Bstring_patt (void *x, void *y);
    extern int Bstring_tag_patt (void *x);
    extern int Barray_tag_patt (void *x);
    extern int Bsexp_tag_patt (void *x);
    extern int Bunboxed_patt (void *x);
    extern int Bboxed_patt (void *x);
    extern int Bclosure_tag_patt (void *x);
    extern void* Bclosure_with_data (int bn, void *entry, int* data_inp);
    extern void* Barray_with_data (int bn, int* data_inp);
    extern void* Bsexp_with_data (int bn, int* data_, int tag);
    extern void* Belem_unboxed (void *p, int i);
    extern void* Bsta (void *v, int i, void *x);
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

        fread(&string_table_size, sizeof(int), 1, f);
        fread(&global_size, sizeof(int), 1, f);
        fread(&public_table_size, sizeof(int), 1, f);
        global_ptr = new int[global_size];

        fseek(f, 0, SEEK_END);
        auto content_size = ftell(f);
        content_size -= 3 * sizeof(int32_t);
        content = new char[content_size];

        fseek(f, 3 * sizeof(int32_t), SEEK_SET);
        fread(content, 1, content_size, f);
        fclose(f);

        string_table_ptr = content + public_table_size * 2 * sizeof(int);
        bytecode_ptr = string_table_ptr + string_table_size;
    }
    ~file_loader() {
        delete[] content;
        delete[] global_ptr;
    }
};

int32_t code(int32_t val) {
    return val * 2 + 1;
}

int32_t decode(int32_t val) {
    return val >> 1;
}

class lama_c_interpreter {
public:
    int32_t* head;
    int max_size = 1024 * 1024;

    file_loader* file;

    char* bytecode;
public:
    lama_c_interpreter(file_loader* file) : file(file), bytecode(file->bytecode_ptr) {
        // stack grows down
        head = (new int[max_size]) + max_size;
        __gc_stack_top = head;
        __gc_stack_bottom = head;
    }

    ~lama_c_interpreter() {
        // delete allocated stack (it grows down)
        delete[] (__gc_stack_top - max_size);
    }

    void push(int32_t val) {
        if (__gc_stack_bottom + max_size == __gc_stack_top) {
            failure((char*)"Illegal push -- too many elements in the stack");
        }
        // stack grows down
        __gc_stack_bottom--;
        *(__gc_stack_bottom) = val;
    }
    int32_t pop() {
        if (__gc_stack_bottom >= head) {
            failure((char*)"Illegal pop -- no elements in the stack");
        }
        // stack grows down
        return *(__gc_stack_bottom++);
    }
    int32_t get_int() {
        int32_t val = *reinterpret_cast<int32_t*>(bytecode);
        bytecode += sizeof(int32_t);
        return val;
    }
    char* get_string() {
        int32_t var = get_int();
        return &file->string_table_ptr[var];
    }
    int32_t* get_lds(char lower_bytes, int32_t id) {
        switch (lower_bytes) {
            case LD_vars::global:
                // global
                return file->global_ptr + id;
            case LD_vars::local:
                // local
                return head - id - 1;
            case LD_vars::arg:
                // arg
                return head + id + 3;
            case LD_vars::closure: {
                // closure
                auto n = *(head + 1);
                auto arg = head + n + 2;
                int32_t* closure = reinterpret_cast<int32_t*>(*arg);
                auto cl_to = Belem_unboxed(closure, code(id + 1));
                return reinterpret_cast<int32_t*>(cl_to);
            }
            default:
                // error
                failure((char*)"incorrect LD operation");
        }
    }
};

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
                // STOP
                return 0;
            case BINOP: {
                // BINOP
                auto right = decode(interp.pop());
                auto left = decode(interp.pop());
                switch (lower_bytes) {
                    case BINOP_ops::plus:
                        interp.push(code(left + right));
                        break;
                    case BINOP_ops::minus:
                        interp.push(code(left - right));
                        break;
                    case BINOP_ops::mul:
                        interp.push(code(left * right));
                        break;
                    case BINOP_ops::div:
                        interp.push(code(left / right));
                        break;
                    case BINOP_ops::mod:
                        interp.push(code(left % right));
                        break;
                    case BINOP_ops::le:
                        interp.push(code(left < right));
                        break;
                    case BINOP_ops::leq:
                        interp.push(code(left <= right));
                        break;
                    case BINOP_ops::ge:
                        interp.push(code(left > right));
                        break;
                    case BINOP_ops::geq:
                        interp.push(code(left >= right));
                        break;
                    case BINOP_ops::eq:
                        interp.push(code(left == right));
                        break;
                    case BINOP_ops::neq:
                        interp.push(code(left != right));
                        break;
                    case BINOP_ops::_and:
                        interp.push(code(left && right));
                        break;
                    case BINOP_ops::_or:
                        interp.push(code(left || right));
                        break;
                    default:
                        // error
                        failure((char *) "incorrect BINOP operation %d", lower_bytes);
                }
                break;
            }
            case Prog_command:
                switch (lower_bytes) {
                    case Prog_comms::CONST:
                        // CONST
                        interp.push(code(interp.get_int()));
                        break;
                    case Prog_comms::STRING: {
                        // STRING
                        auto val = interp.get_int();
                        auto str = reinterpret_cast<int32_t>(Bstring(interp.file->string_table_ptr + val));
                        interp.push(str);
                        break;
                    }
                    case Prog_comms::SEXP: {
                        // SEXP
                        char *str = interp.get_string();
                        auto len = interp.get_int(), tag = LtagHash(str);

                        int32_t *last = __gc_stack_bottom, *first = last + len - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto res = reinterpret_cast<int32_t>(Bsexp_with_data(code(len + 1), __gc_stack_bottom, tag));
                        __gc_stack_bottom += len;
                        interp.push(res);
                        break;
                    }
                    case Prog_comms::STI:
                        // STI
                        // No impl
                        break;
                    case Prog_comms::STA: {
                        // STA
                        auto val = reinterpret_cast<void*>(interp.pop());
                        auto ind = interp.pop();
                        void* to;
                        if (ind % 2 != 1) {
                            to = Bsta(val, ind, nullptr);
                        } else {
                            auto ds = reinterpret_cast<void*>(interp.pop());
                            to = Bsta(val, ind, ds);
                        }
                        interp.push(reinterpret_cast<int32_t>(to));
                        break;
                    }
                    case Prog_comms::JMP: {
                        // JMP
                        auto len = interp.get_int();
                        interp.bytecode = interp.file->bytecode_ptr + len;
                        break;
                    }
                    case Prog_comms::END: {
                        // END
                        auto rest = interp.pop();
                        __gc_stack_bottom = interp.head;
                        interp.head = reinterpret_cast<int32_t*>(*(__gc_stack_bottom++));
                        if (__gc_stack_bottom == interp.head) {
                            interp.bytecode = nullptr;
                            break;
                        }
                        auto n = interp.pop();
                        char* ext = reinterpret_cast<char*>(interp.pop());
                        __gc_stack_bottom += n;
                        interp.push(rest);
                        interp.bytecode = ext;
                        break;
                    }
                    case Prog_comms::RET:
                        // RET
                        // No impl
                        break;
                    case Prog_comms::DROP:
                        // DROP
                        interp.pop();
                        break;
                    case Prog_comms::DUP: {
                        // DUP
                        auto val = interp.pop();
                        interp.push(val);
                        interp.push(val);
                        break;
                    }
                    case Prog_comms::SWAP:
                        // SWAP
                        // No impl
                        break;
                    case Prog_comms::ELEM: {
                        // ELEM
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
                // LD
                auto id = interp.get_int();
                int32_t* val = interp.get_lds(lower_bytes, id);
                interp.push(*val);
                break;
            }
            case LDA: {
                // LDA
                auto id = interp.get_int();
                int32_t* val = interp.get_lds(lower_bytes, id);
                interp.push(reinterpret_cast<int32_t>(val));
                break;
            }
            case ST: {
                // ST
                auto id = interp.get_int();
                auto val = interp.pop();
                *interp.get_lds(lower_bytes, id) = val;
                interp.push(val);
                break;
            }
            case Func_command:
                switch (lower_bytes) {
                    case Func_comms::CJMPz: {
                        // CJMPz
                        auto len = interp.get_int();
                        auto check = interp.pop();
                        if (decode(check) == 0) {
                            interp.bytecode = interp.file->bytecode_ptr + len;
                        }
                        break;
                    }
                    case Func_comms::CJMPnz: {
                        // CJMPnz
                        auto len = interp.get_int();
                        auto check = interp.pop();
                        if (decode(check) != 0) {
                            interp.bytecode = interp.file->bytecode_ptr + len;
                        }
                        break;
                    }
                    case Func_comms::BEGIN: {
                        // BEGIN
                        interp.get_int();
                        auto n = interp.get_int();
                        interp.push(reinterpret_cast<int32_t>(interp.head));
                        interp.head = __gc_stack_bottom;
                        for (int i = 0; i < n; i++) {
                            interp.push(code(0));
                        }
                        break;
                    }
                    case Func_comms::cBEGIN: {
                        // cBEGIN
                        interp.get_int();
                        auto n = interp.get_int();
                        interp.push(reinterpret_cast<int32_t>(interp.head));
                        interp.head = __gc_stack_bottom;
                        for (int i = 0; i < n; i++) {
                            interp.push(code(0));
                        }
                        break;
                    }
                    case Func_comms::CLOSURE: {
                        // CLOSURE
                        auto len = interp.get_int(), n = interp.get_int();
                        int32_t bind[n];
                        for (int i = 0; i < n; i++) {
                            char c = *interp.bytecode;
                            interp.bytecode++;
                            auto val = interp.get_int();
                            bind[i] = *interp.get_lds(c, val);
                        }
                        auto closure = Bclosure_with_data(code(n), interp.file->bytecode_ptr + len, bind);
                        interp.push(reinterpret_cast<int32_t>(closure));
                        break;
                    }
                    case Func_comms::CALLC: {
                        // CALLC
                        auto n = interp.get_int();
                        auto el = reinterpret_cast<int32_t*>(__gc_stack_bottom[n]);
                        auto elem = Belem(el, code(0));
                        char* callc = reinterpret_cast<char*>(elem);
                        int32_t *last = __gc_stack_bottom, *first = last + n - 1;
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
                    case Func_comms::CALL_2_args: {
                        // CALL with 2 args
                        auto len = interp.get_int(), n = interp.get_int();
                        int32_t *last = __gc_stack_bottom, *first = last + n - 1;
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
                        // TAG
                        auto str = interp.get_string();
                        auto n = interp.get_int();
                        auto hash = LtagHash(str);
                        auto store = interp.pop();
                        void* data = reinterpret_cast<void*>(store);
                        auto tag = Btag(data, hash, code(n));
                        interp.push(tag);
                        break;
                    }
                    case Func_comms::ARRAY: {
                        // ARRAY
                        auto n = interp.get_int();
                        int32_t* pt = reinterpret_cast<int32_t*>(interp.pop());
                        auto arr = Barray_patt(pt, code(n));
                        interp.push(arr);
                        break;
                    }
                    case Func_comms::FAIL: {
                        // FAIL
                        failure((char*)"Failed with FAIL");
                        break;
                    }
                    case Func_comms::LINE:
                        // LINE
                        interp.get_int();
                        break;
                    default:
                        // error
                        failure((char*)"incorrect operation");
                }
                break;
            case PATT: {
                // PATT
                int32_t* patt = reinterpret_cast<int32_t*>(interp.pop());
                switch (lower_bytes) {
                    case PATT_comms::StrCmp: {
                        // StrCmp
                        int32_t* pt = reinterpret_cast<int32_t*>(interp.pop());
                        interp.push(Bstring_patt(patt, pt));
                        break;
                    }
                    case PATT_comms::String:
                        // String
                        interp.push(Bstring_tag_patt(patt));
                        break;
                    case PATT_comms::Array:
                        // Array
                        interp.push(Barray_tag_patt(patt));
                        break;
                    case PATT_comms::Sexp:
                        // Sexp
                        interp.push(Bsexp_tag_patt(patt));
                        break;
                    case PATT_comms::Boxed:
                        // Boxed
                        interp.push(Bboxed_patt(patt));
                        break;
                    case PATT_comms::UnBoxed:
                        // UnBoxed
                        interp.push(Bunboxed_patt(patt));
                        break;
                    case PATT_comms::Closure:
                        // Closure
                        interp.push(Bclosure_tag_patt(patt));
                        break;
                    default:
                        // error
                        failure((char*)"incorrect PATT operation");
                }
                break;
            }
            case Lcommand:
                switch (lower_bytes) {
                    case Lcomms::Lread:
                        // Lread
                        interp.push(Lread());
                        break;
                    case Lcomms::Lwrite: {
                        // Lwrite
                        auto val = interp.pop();
                        interp.push(Lwrite(val));
                        break;
                    }
                    case Lcomms::Llength: {
                        // Llength
                        void* val = reinterpret_cast<void*>(interp.pop());
                        interp.push(Llength(val));
                        break;
                    }
                    case Lcomms::Lstring: {
                        // Lstring
                        void* val = reinterpret_cast<void*>(interp.pop());
                        auto str = Lstring(val);
                        auto cast = reinterpret_cast<int32_t>(str);
                        interp.push(cast);
                        break;
                    }
                    case Lcomms::Larray: {
                        // .array
                        auto n = interp.get_int();
                        int32_t *last = __gc_stack_bottom, *first = last + n - 1;
                        while (last < first) {
                            std::swap(*last, *first);
                            // stack grows down
                            last++;
                            first--;
                        }
                        auto arr = Barray_with_data(code(n), __gc_stack_bottom);
                        __gc_stack_bottom += n;
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
