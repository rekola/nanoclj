#ifndef _NANOCLJ_INTERPRETER_H_
#define _NANOCLJ_INTERPRETER_H_

#include "nanoclj.h"
#include "nanoclj_priv.h"

#include <vector>
#include <string>
#include <string_view>
#include <functional>

namespace nanoclj {
  using UserFn = nanoclj_val_t(*)(nanoclj_t *, nanoclj_val_t);
  
  class Value {
  public:
    Value() { }

    int integerValue() {
      return 0;
    }
  };
  
  class Interpreter {
  public:
    Interpreter();
    virtual ~Interpreter();

    Value * eval(const std::string & expression);
    void eval_print(const std::string & expression);

    virtual void print(std::string_view s) { }
    virtual void print_error(std::string_view s) { }

    virtual void set_color(int r, int g, int b) { }
    virtual void reset_color() { }

    void flush() {
      print(out_buffer);
      out_buffer.clear();
    }
    
    void add_chars(std::string_view s) {
      out_buffer += s;
      for (int i = static_cast<int>(out_buffer.size()) - 1; i >= 0; i--) {
	if (out_buffer[i] == '\n') {
	  fprintf(stderr, "flushing out\n");
	  print(std::string_view(out_buffer).substr(0, i + 1));
	  out_buffer.erase(0, i + 1);
	}
      }
    }
    void add_error_chars(std::string_view s) {
      error_buffer += s;
      for (int i = static_cast<int>(error_buffer.size()) - 1; i >= 0; i--) {
	if (error_buffer[i] == '\n') {
	  print_error(std::string_view(error_buffer).substr(0, i + 1));
	  error_buffer.erase(0, i + 1);
	}
      }
    }
    
    void define(std::string symbol, UserFn function) {
      sc_->vptr->nanoclj_intern(sc_, sc_->global_env,
				sc_->vptr->mk_symbol(sc_, symbol.c_str()), 
				sc_->vptr->mk_foreign_func(sc_, function));
    }

    const std::vector<std::string> & getSymbols() const { return symbols_; }
    
  private:
    nanoclj_t * sc_;
    std::vector<std::string> symbols_;
    std::string out_buffer, error_buffer;
  };
};

#endif
