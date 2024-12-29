#ifndef _NANOCLJ_INTERPRETER_H_
#define _NANOCLJ_INTERPRETER_H_

#include "nanoclj.h"
#include "nanoclj_prim.h"
#include "nanoclj-private.h"

#include <vector>
#include <string>
#include <string_view>
#include <functional>

namespace nanoclj {
  using UserFn = nanoclj_val_t(*)(nanoclj_t *, nanoclj_cell_t *);
  
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
    virtual void print_image(imageview_t image) { }

    virtual void set_color(double r, double g, double b) { }
    virtual void restore() { }

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
      sc_->vptr->intern(sc_, sc_->core_ns,
			sc_->vptr->def_symbol(symbol.c_str()), 
			sc_->vptr->mk_foreign_func(sc_, function, 0, -1, NULL)
			);
    }

    const std::vector<std::string> & getSymbols() const { return symbols_; }
    
  private:
    void print_value(nanoclj_val_t v);

    nanoclj_t * sc_;
    std::vector<std::string> symbols_;
    std::string out_buffer, error_buffer;
  };
};

#endif
