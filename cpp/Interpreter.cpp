#include "Interpreter.h"

#include <unistd.h>

using namespace nanoclj;

static void out_print_callback(const char * s, size_t len, void * data) {
  if (data && len > 0) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->add_chars(std::string_view(s, len));
  }
}

static void out_color_callback(double r, double g, double b, void * data) {
  if (data) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->flush();
    interpreter->set_color(r, g, b);
  }
}

static void out_restore_callback(void * data) {
  if (data) {
    auto interpreter = reinterpret_cast<Interpreter *>(data);
    interpreter->flush();
    interpreter->restore();
  }
}

static void out_image_callback(imageview_t image, void * data) {
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
    nanoclj_set_output_port_callback(sc, &out_print_callback, &out_color_callback, &out_restore_callback, &out_image_callback);
    nanoclj_set_error_port_callback(sc, &error_callback);
    nanoclj_set_external_data(sc, this);
    nanoclj_set_object_invoke_callback(sc, &object_invoke_callback);

    char buf[1024];
    const char * path = getcwd(buf, 1024);
    if (path) {
      chdir("assets/nanoclj");
      nanoclj_load_named_file(sc, "init.clj");
      chdir(path);
    }

#if 0
    std::string expr = "(ns-interns *ns*)";
    auto sym = nanoclj_eval_string(sc, expr.c_str(), expr.size());
    for ( ; sym.as_long != sc->EMPTY.as_long; sym = sc->vptr->pair_cdr(sym)) {
      auto v = sc->vptr->pair_car(sym);
      if (sc->vptr->is_symbol(v)) {
	auto sym0 = sc->vptr->to_strview(v);
	auto sym = std::string_view(sym0.ptr, sym0.size);
	symbols_.emplace_back(sym);
      } else if (sc->vptr->is_mapentry(v)) {
	auto sym0 = sc->vptr->to_strview(sc->vptr->pair_car(v));
	auto sym = std::string_view(sym0.ptr, sym0.size);
	symbols_.emplace_back(sym);
      }
    }
#endif
  }
}

Interpreter::~Interpreter() {
  nanoclj_deinit(sc_);
  free(sc_);
}

Value *
Interpreter::eval(const std::string & expression) {
  nanoclj_eval_string(sc_, expression.c_str(), expression.size());
  return nullptr;
}

void
Interpreter::eval_print(const std::string & expression) {
  nanoclj_val_t v = nanoclj_eval_string(sc_, expression.c_str(), expression.size());
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
