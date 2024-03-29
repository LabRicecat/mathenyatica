
#define mny_ADV_CONFIGS
#include "mathenyatica.hpp"
#include "catpkgs/argparser/argparser.h"
#include <readline/readline.h>

std::tuple<std::string,std::string> split_sh(std::string src) {
    int i = 0;
    while(i < src.size() && (src[i] != ' ' || src[i] == '\t')) ++i;
    std::string c1 = src.substr(0,i);
    std::string c2 = src.substr(i,src.size()-1);
    c1.erase(c1.find_last_not_of(" \t\n\r\f\v") + 1);
    c1.erase(0, c1.find_first_not_of(" \t\n\r\f\v"));
    c2.erase(c2.find_last_not_of(" \t\n\r\f\v") + 1);
    c2.erase(0, c2.find_first_not_of(" \t\n\r\f\v"));

    return std::make_tuple(c1,c2);
}

std::string rd(std::string file) {
    std::ifstream ifs(file);
    std::string r;
    while(ifs.good()) r += ifs.get();
    if(!r.empty()) r.pop_back();

    return r;
}

template<typename ...Targs>
bool is_of(std::string c, Targs ...args) {
    std::vector<std::string> a = {args...};
    for(auto i : a) if(i == c) return true;
    return false;
}

#define errc() if(true) { if(_mny_err) { std::cout << "Error: " << mny_error() << "\n"; continue; } } else do {} while(0)
#define hasarg() if(true) { if(arg == "") { std::cout << "Command requires an argument!\n"; continue; } } else do {} while(0)
#define noarg() if(true) { if(arg != "") { std::cout << "Command does not allow an argument!\n"; continue; } } else do {} while(0)
int main(int argc, char** argv) {
    std::string layout = "$> ", input;
    std::vector<mny_function> functions;
    
    if(argc >= 2) {
        std::string arg;
        for(size_t i = 1; i < argc; ++i) {
            arg = std::string(argv[i]);
            if(arg.front() == '-') {
                arg.erase(arg.begin());
                for(auto c : arg) {
                    switch(c) {
                        case 'e':
                            mny::eval_show = true;
                            break;
                        case 'v':
                            mny::val_show = true;
                            break;
                        case 'r':
                            mny::run_show = true;
                            break;
                        case 'a':
                            mny::eval_show = true;
                            mny::val_show = true;
                            mny::run_show = true;
                            break;
                        case 'h':
                            std::cout << "# Mathenyatica shell\n\n"
                                << "  Usage: mathenyatica [options] [files]\n\n"
                                << "Loads FILES if any, and opens the mathenyatica shell.\n"
                                << "Options: \n"
                                << " -e : shows all eval() steps\n"
                                << " -v : shows all val() steps\n"
                                << " -r : shows all runf() steps\n"
                                << " -a : all the 3 above\n"
                                << " -h : this\n"
                                << "Note: `-v -e` is the same as `-ve`\n";
                            std::exit(0);
                            break;
                        default:
                            std::cout << "unknown option: " << c << "\n"
                            << "  try -h for help!\n";
                            std::exit(1);
                            break;
                    }
                }
            }
            else {
                auto fns = mny_parse(rd(arg));
                if(_mny_err) { std::cout << mny_error() << "\n"; std::exit(1); }
                for(auto i : fns) functions.push_back(i);
            }
        }
    }
    
    std::cout << " --- Mathenyatica Shell ---\n"
            << "Use `help` for help.\n"
            << "LabRicecat (c) 2023\n\n";

    while(true) {
        input = readline(layout.c_str());
        auto [command, arg] = split_sh(input);

        if(is_of(command,"file","f","fi")) {
            hasarg();
            auto fns = mny_parse(rd(arg));
            errc();
            for(auto i : fns) functions.push_back(i);
        }
        else if(is_of(command,"clear","c","cls")) {
            noarg();
            functions.clear();
            std::cout << "Cleared functions\n";
        }
        else if(is_of(command,"info","i","in")) {
            hasarg();
            mny_function f = mny_getf(arg,functions);
            errc();
            std::cout << f.name << " ";
            std::string out;
            for(auto b : f.body) {
                for(auto i : b) {
                    if(i.front() == '(' || i.front() == '<') if(out.back() == ' ') out.pop_back();
                    out += i;
                    if(i != "!") out += " ";
                }
                if(out.back() == ' ') out.pop_back();
                out += ", ";
            }
            if(out != "") { out.pop_back(); out.pop_back(); }
            std::cout << out << "\n";
        }
        else if(is_of(command,"eval","e","ev")) {
            hasarg();
            std::cout << (mny_eval(arg,"",functions,{}) ? "TRUE" : "FALSE") << "\n";
            errc();
        }
        else if(is_of(command,"view","v","all")) {
            hasarg();
            std::string cm;
            KittenLexer lexer = KittenLexer()
                .add_ignore(' ')
                .add_ignore('\t')
                .add_capsule('(',')')
                .add_capsule('<','>')
                .erase_empty()
                .add_lineskip(';')
                .add_linebreak('\n');
            auto lexed = lexer.lex(arg);
            if(lexed.size() != 2 || !mny_isf(lexed[0].src,functions) || lexed[1].src[0] != '(') {
                std::cout << "Expected function call\n";
                continue;
            }
            mny_function f = mny_getf(lexed[0].src,functions);

            std::string args;
            lexed[1].src.erase(lexed[1].src.begin());
            lexed[1].src.pop_back();
            auto parsed_args = mny_argparse(lexed[1].src,"",functions,{});

            for(size_t i = 0; i < f.body.size(); ++i) {
                std::cout << f.name << "<" << (i+1) << ">: " << (mny_runf(f.name,functions,parsed_args,i) ? "TRUE" : "FALSE") << "\n";
            }
        }
        else if(is_of(command,"def","d","fn")) {
            auto fns = mny_parse(arg);
            errc();
            for(auto i : fns) functions.push_back(i);
        }
        else if(is_of(command,"quit","q","exit",":q",":q!")) {
            break;
        }
        else if(is_of(command,"help","h")) {
            std::cout << 
                "file <file.mny>    : loads a file\n" <<
                "clear              : clears all data\n" <<
                "eval <statement>   : evaluates a statement\n" <<
                "def|fn <function>  : defined a function\n" <<
                "quit|exit          : exits the program\n" <<
                "view <functioncall>: shows all return values of a function\n" <<
                "info <function>    : shows information about a function\n" <<
                "help               : this\n";
        }
        else if(command != "") {
            std::cout << "Unknown \"" + command + "\"\n";
        }
    }
}