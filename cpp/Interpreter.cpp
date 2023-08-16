#include "Interpreter.h"

#include <unistd.h>

using namespace nanoclj;

static void out_print_callback(const char * s, size_t len, void * data) {
  if (data && len > 0) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->add_chars(std::string_view(s, len));
  }
}

static void out_color_callback(int r, int g, int b, void * data) {
  if (data) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->flush();
    interpreter->set_color(r, g, b);
  }
}

static void out_reset_color_callback(void * data) {
  if (data) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->flush();
    interpreter->reset_color();
  }  
}

static void out_image_callback(nanoclj_image_t * image, void * data) {
  if (data) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->flush();
    interpreter->print_image(image);
  }
}

static void error_callback(const char * s, size_t len, void * data) {
  if (len > 0) {
    auto clojure = reinterpret_cast<Interpreter *>(data);
    clojure->add_error_chars(std::string_view(s, len));
  }
}

static nanoclj_val_t object_invoke_callback(nanoclj_t * sc, void * object, nanoclj_val_t args) {
  return args;
}

Interpreter::Interpreter() {
  auto sc = nanoclj_init_new();
  if (!sc) {
    fprintf(stderr, "initialization failed\n");
  } else {
    sc_ = sc;

    nanoclj_set_input_port_file(sc, stdin);
    nanoclj_set_output_port_callback(sc, &out_print_callback, &out_color_callback, &out_reset_color_callback, &out_image_callback);
    nanoclj_set_error_port_callback(sc, &error_callback);
    nanoclj_set_external_data(sc, this);
    nanoclj_set_object_invoke_callback(sc, &object_invoke_callback);
    
    chdir("assets/nanoclj");
    auto fin = fopen("init.clj", "rt");
    if (fin) {
      nanoclj_load_named_file(sc, fin, "init.clj");
      fclose(fin);
    }
    
    auto sym = nanoclj_eval_string(sc, "(ns-interns *ns*)");
    for ( ; sym.as_uint64 != sc->EMPTY.as_uint64; sym = sc->vptr->pair_cdr(sym)) {
      auto v = sc->vptr->pair_car(sym);
      if (sc->vptr->is_symbol(v)) {
	symbols_.emplace_back(sc->vptr->symname(v));
      } else if (sc->vptr->is_mapentry(v)) {
	symbols_.emplace_back(sc->vptr->symname(sc->vptr->pair_car(v)));
      }
    }
  }
}

Interpreter::~Interpreter() {
  nanoclj_deinit(sc_);
  free(sc_);
}

Value *
Interpreter::eval(const std::string & expression) {
  nanoclj_eval_string(sc_, expression.c_str());
  return nullptr;
}

void
Interpreter::eval_print(const std::string & expression) {
  nanoclj_val_t v = nanoclj_eval_string(sc_, expression.c_str());
  nanoclj_val_t q = sc_->vptr->cons(sc_, sc_->QUOTE, sc_->vptr->cons(sc_, v, sc_->EMPTY));
  nanoclj_val_t c =
    sc_->vptr->cons(sc_,
		    sc_->vptr->def_symbol(sc_, "prn"),
		    sc_->vptr->cons(sc_,
				    q, sc_->EMPTY));
  nanoclj_eval(sc_, c);
}

#if 0
void
Interpreter::execute(const std::string & expression) {
  nanoclj_load_string(sc_, expression.c_str());
}
#endif
