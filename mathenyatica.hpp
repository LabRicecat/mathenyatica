#ifndef MATHENYATICA_HPP
#define MATHENYATICA_HPP

#include "catpkgs/kittenlexer/kittenlexer.hpp"

#include <fstream>

#ifdef mny_ADV_CONFIGS

namespace mny {
    inline static bool eval_show = false;
    inline static bool val_show = false;
    inline static bool run_show = false;

    inline static std::string body_out(std::vector<std::string> body) {
        std::string out;
        for(auto i : body) {
            if(i.front() == '(' || i.front() == '<') if(out.back() == ' ') out.pop_back();
            out += i;
            if(i != "!") out += " ";
        }
        return out;
    }
}

#endif

using mny_result_t = bool;

struct mny_function {
    using body_t = std::vector<std::vector<std::string>>;
    int argc = 0;
    std::string name;
    body_t body;
};

struct mny_error_t {
    std::string message;

    operator bool() { return !message.empty(); }
}inline static _mny_err;

struct mny_builtin {
    int argc = 0;
    std::vector<mny_result_t> (*body)(std::vector<std::string>) = 0;
};

template<typename ..._Targs>
inline static mny_result_t mny_run(std::string function, std::vector<mny_function> functions, _Targs ...targs);
inline static mny_result_t mny_eval(std::string src, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args);
inline static mny_result_t mny_eval(std::vector<std::string> body, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args);
inline static mny_result_t mny_runf(std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args, int result);
inline static mny_result_t mny_eval_f(std::vector<std::string> body, std::string function, size_t& idx, std::vector<mny_function> functions, std::vector<mny_result_t> args);

inline static mny_function mny_parse_funame(std::string token, int line) {
    mny_function f;
    size_t i;
    for(i = 0; i < token.size() && !(token[i] >= '0' && token[i] <= '9'); ++i) {
        f.name += token[i];
    }

    if(i == 0) {
        _mny_err.message = "line " + std::to_string(line) + ": invalid function name \"" + token + "\"";
        return {};
    };

    if(f.name != token) {
        f.argc = std::stoi(token.substr(i, token.size()-1));
        f.name += token.substr(i, token.size()-1);
    } 
    else f.argc = 0;

    return f;
}

inline static std::vector<mny_function> mny_parse(std::string source) {
    KittenLexer lexer = KittenLexer()
        .add_ignore(' ')
        .add_ignore('\t')
        .add_capsule('(',')')
        .add_capsule('<','>')
        .add_extract('&')
        .add_extract('!')
        .erase_empty()
        .add_lineskip(';')
        .add_extract(',')
        // .add_backslashopt('\n',' ')
        .add_linebreak('\n');
    auto lexed = lexer.lex(source);

    std::vector<std::vector<std::string>> lines;
    int cline = 0;
    for(auto i : lexed) {
        if(i.line != cline) {
            cline = i.line;
            lines.push_back({});
        }
        lines.back().push_back(i.src);
    }

    std::vector<mny_function> funs;
    for(size_t line = 0; line < lines.size(); ++line) {
        if(lines[line][0] == "%") {
            std::ifstream rd;
            for(size_t i = 1; i < lines[line].size(); ++i) {
                rd.open(lines[line][i]);
                if(!rd.is_open()) {
                    _mny_err.message = "no such file: " + lines[line][i];
                    return {};
                }
                std::string in;
                while(rd.good()) in += rd.get();
                if(in != "") in.pop_back();

                auto fns = mny_parse(in);
                for(auto f : fns) funs.push_back(f);
            }
        }
        mny_function fun = mny_parse_funame(lines[line][0],line);
        if(_mny_err) return {};

        auto bodies = lines[line];
        bodies.erase(bodies.begin());

        fun.body.push_back({});
        for(auto i : bodies) {
            if(i == ",") 
                fun.body.push_back({});
            else fun.body.back().push_back(i);
        }

        funs.push_back(fun);
    }

    return funs;
}

inline static std::string mny_error() {
    std::string r = _mny_err.message;
    _mny_err.message = "";
    return r;
}

inline static bool mny_isf(std::string function, std::vector<mny_function> functions) {
    for(auto i : functions) if(i.name == function) return true;
    return false;
}

inline static mny_function mny_getf(std::string function, std::vector<mny_function> functions) {
    mny_function f;
    for(auto i : functions) if(i.name == function) f = i;
    if(f.name == "") { _mny_err.message = "unknown function \"" + function + "\""; return {}; }
    return f;
}

inline static bool mny_isnum(std::string src) {
    for(auto i : src) if(i < '0' || i > '9') return false;
    return true;
}

inline static mny_result_t mny_val(std::string src, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args) {
    size_t i;
    for(i = 0; i < src.size() && src[i] == '!'; ++i) src.erase(src.begin());
    mny_result_t res;
    if(mny_isnum(src)) {
        if(std::stoi(src) > args.size()) {
            _mny_err.message = "unknown argument: " + src;
            return false;
        }
        res = args[std::stoi(src) - 1];
    }
    else if(src == "TRUE") return true;
    else if(src == "FALSE") return false;
    else if(src.front() == '(') {
        src.pop_back();
        src.erase(src.begin());
        res = mny_eval(src,function,functions,args);
    }
    else if(src.front() == '#') {
        if(function == "") {
            _mny_err.message = "using return value reference outside function";
            return false;
        }
        src.erase(src.begin());
        if(!mny_isnum(src)) {
            _mny_err.message = "not a number as return value reference";
            return false;
        }
        int res = std::stoi(src) - 1;
        mny_function f = mny_getf(function,functions);
        if(res >= f.body.size()) {
            _mny_err.message = "no such return value: " + std::to_string(res) + " " + function;
            return false;
        }
        return mny_eval(f.body[res],f.name,functions,args);
    }
    else if(mny_isf(src,functions)) {
        mny_function f = mny_getf(src,functions);
        if(f.argc != 0) {
            _mny_err.message = "unexpected function name: " + src;
            return false;
        }
        res = mny_run(src,functions);
    }
    else {
        _mny_err.message = "unexpected token: " + src;
        return false;
    }
#ifdef mny_ADV_CONFIGS
    if(mny::val_show)
        std::cout << "[LL val()]: " << src << " -> " << ((i % 2 == 0 ? res : !res) ? "TRUE" : "FALSE") << "\n";
#endif
    return i % 2 == 0 ? res : !res;
}

inline static std::vector<mny_result_t> mny_argparse(std::string src, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args) {
    KittenLexer lexer = KittenLexer()
        .add_ignore(' ')
        .add_ignore('\t')
        .add_capsule('(',')')
        .add_capsule('<','>')
        .erase_empty()
        .add_lineskip(';')
        .add_linebreak('\n');
    auto lexed = lexer.lex(src);
    std::vector<std::string> body;
    for(auto i : lexed) body.push_back(i.src);
    
    std::vector<mny_result_t> pargs;

    for(size_t i = 0; i < body.size(); ++i) {
        if(body[i].front() == '(') {
            body[i].erase(body[i].begin());
            body[i].pop_back();
            pargs.push_back(mny_eval(body[i],function,functions,args));
        }
        else if(mny_isf(body[i],functions)) {
            mny_function f = mny_getf(body[i],functions);
            auto val = mny_eval_f(body,function,i,functions,args);
            if(_mny_err) return {};
            pargs.push_back(val);
        } 
        else pargs.push_back(mny_val(body[i],function,functions,args));

        if(_mny_err) return {};
    }
    return pargs;
}

inline static mny_result_t mny_eval(std::string src, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args) {
    KittenLexer lexer = KittenLexer()
        .add_ignore(' ')
        .add_ignore('\t')
        .add_capsule('(',')')
        .add_capsule('<','>')
        .add_extract('&')
        .add_extract('!')
        .erase_empty()
        .add_lineskip(';')
        .add_linebreak('\n');
    auto lexed = lexer.lex(src);
    std::vector<std::string> body;

    for(auto i : lexed) body.push_back(i.src);
    
    return mny_eval(body,function, functions,args);
}

inline static mny_result_t mny_eval_f(std::vector<std::string> body, std::string function, size_t& idx, std::vector<mny_function> functions, std::vector<mny_result_t> args) {
    mny_function f = mny_getf(body[idx],functions);
    if(_mny_err) return false;
    if(idx + 1 < body.size()) {
        auto parens = body[++idx];
        int return_val = 0;
        if(parens.front() == '<') {
            parens.pop_back();
            parens.erase(parens.begin());
            if(!mny_isnum(parens)) {
                _mny_err.message = "expected constant in <>";
                return {};
            }
            return_val = std::stoi(parens) - 1;
            if(idx + 1 >= body.size() && f.argc != 0) {
                _mny_err.message = "no parameterlist!";
                return {};
            }
                    
            parens = body[++idx];
        }
        std::vector<mny_result_t> _args;
        if(parens.front() == '(') {
            parens.pop_back();
            parens.erase(parens.begin());
            _args = mny_argparse(parens,function,functions,args);
            if(_mny_err) return {};
            if(f.argc > _args.size()) {
                _mny_err.message = "function " + f.name + ": not enough arguments";
                return {};
            }
            else if(f.argc < _args.size()) {
                _mny_err.message = "function " + f.name + ": too many arguments";
                return {};
            }
        }
        else --idx;

        return mny_eval(f.body[return_val],f.name,functions,_args);
    }
    else if(f.argc == 0) {
        return mny_eval(f.body[0],f.name,functions,{});
    }
    else {
        _mny_err.message = "no parameter for funtion " + f.name;
        return false;
    }
}

inline static mny_result_t mny_eval(std::vector<std::string> body, std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args) {
#ifdef mny_ADV_CONFIGS
    if(mny::eval_show)
        std::cout << "[LL eval()]: " << mny::body_out(body) << "\n";
#endif
    mny_result_t lhs;
    bool lhsdef = false;
    int not_f = 0;
    int and_f = 0;
    for(size_t i = 0; i < body.size(); ++i) {
        if(body[i] == "!") ++not_f;
        else if(body[i] == "&" && (and_f || not_f || !lhsdef)) { _mny_err.message = "invalid & as token " + std::to_string(i+1); return false; }
        else if(body[i] == "&") ++and_f;
        else if(mny_isf(body[i],functions)) {
            auto val = mny_eval_f(body,function,i,functions,args);
            if(_mny_err) return false;
            if(not_f % 2 == 1) lhs = !lhs;
            not_f = 0;
            if(lhsdef) {
                not_f = 0;
                if(and_f) {
                    lhs = lhs & val;
                    if(!lhs) return false;
                    and_f = 0;
                }
                else {
                    _mny_err.message = "invalid syntax";
                    return false;
                }
            }
            else {
                lhs = val;
                lhsdef = true;
            }   
        }
        else {
            if(lhsdef) {
                auto val = mny_val(body[i],function, functions,args);
                if(_mny_err) return false;
                if(not_f % 2 == 1) val = !val;
                not_f = 0;
                if(and_f) {
                    lhs = lhs & val;
                    if(!lhs) return false;
                    and_f = 0;
                }
                else {
                    _mny_err.message = "invalid syntax";
                    return false;
                }
            }
            else {
                lhs = mny_val(body[i],function,functions,args);
                if(_mny_err) return false;
                if(not_f % 2 == 1) lhs = !lhs;
                not_f = 0;
                lhsdef = true;
                if(!lhs) return false;
            }   
        }
    }
    if(not_f || and_f) {
        _mny_err.message = "early exit";
        return false;
    }
    return lhs;
}

template<typename ..._Targs>
inline static mny_result_t mny_run(std::string function, std::vector<mny_function> functions, _Targs ...targs) {
    std::vector<mny_result_t> args = {targs...};

    return mny_runf(function,functions,args,0);
}

inline static mny_result_t mny_runf(std::string function, std::vector<mny_function> functions, std::vector<mny_result_t> args, int result) {
#ifdef mny_ADV_CONFIGS
    if(mny::run_show) {
        std::cout << "[LL runf()]: " << function << "( ";
        for(auto i : args) std::cout << (i ? "TRUE" : "FALSE") << " ";
        std::cout << ")\n";
    }
#endif
    mny_function f = mny_getf(function,functions);
    if(_mny_err) { return false; }

    if(f.argc > args.size()) {
        _mny_err.message = "function " + f.name + ": not enough arguments";
        return false;
    }
    else if(f.argc < args.size()) {
        _mny_err.message = "function " + f.name + ": too many arguments";
        return false;
    }
    else if(result >= f.body.size()) {
        _mny_err.message = "function " + f.name + ": no such return value: " + std::to_string(result + 1);
        return false;
    }
    else if(result < 0) {
        _mny_err.message = "function " + f.name + ": no such return value: " + std::to_string(result + 1);
        return false;
    }

    return mny_eval(f.body[result],f.name,functions,args);
}

// OOP Syntax Sugar
struct mny_interpreter {
    std::vector<mny_function> functions;

    template<typename ...Targs>
    mny_result_t run_function(std::string function, Targs ...targs) {
        std::vector<mny_result_t> args = {targs...};
        return mny_runf(function,functions,args,0);
    }
    template<int _Tres, typename ...Targs>
    mny_result_t run_function(std::string function, Targs ...targs) {
        std::vector<mny_result_t> args = {targs...};
        return mny_runf(function,functions,args,_Tres);
    }
    template<typename ...Targs>
    mny_result_t run_function(std::string function, int result, Targs ...targs) {
        std::vector<mny_result_t> args = {targs...};
        return mny_runf(function,functions,args,result);
    }
    template<typename ...Targs>
    std::vector<mny_result_t> run_function_all(std::string function,  Targs ...targs) {
        std::vector<mny_result_t> args = {targs...};
        std::vector<mny_result_t> results;
        mny_function f = mny_getf(function, functions);
        for(size_t i = 0; i < f.body.size(); ++i) {
            results.push_back(mny_runf(function,functions,args,i));
            if(_mny_err) return {};
        }
        return results;
    }

    mny_interpreter& parse(std::string src) {
        auto fns = mny_parse(src);
        for(auto i : fns) functions.push_back(i);
        return *this;
    }

    mny_interpreter& clear() {
        functions.clear();
        return *this;
    }

    mny_result_t eval(std::string expression) {
        return mny_eval(expression,"",functions,{});
    }

    mny_function get_function(std::string name) {
        return mny_getf(name,functions);
    }

    bool is_function(std::string name) {
        return mny_isf(name,functions);
    }
};

#endif