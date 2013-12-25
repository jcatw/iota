#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef BUFFER_MAX
#define BUFFER_MAX 1024
#endif

/************/
/* language */
/************/

typedef enum {NIL, BOOLEAN, SYMBOL, FIXNUM,
              CHARACTER, STRING, CONS,
              PRIMITIVE_PROC, COMPOUND_PROC} object_type;

typedef struct object {
  object_type type;
  union {
    struct {
      char value;
    } boolean;
    struct {
      char *value;
    } symbol;
    struct {
      long value;
    } fixnum;
    struct {
      char value;
    } character;
    struct {
      char *value;
    } string;
    struct {
      struct object *first;
      struct object *rest;
    } cons;
    struct {
      struct object * (*fn)(struct object *args);
    } primitive_proc;
    struct {
      struct object *parameters;
      struct object *body;
      struct object *env;
    } compound_proc;
  } data;
} object;

object *alloc_object() {
  object *obj;

  obj = (object *) malloc(sizeof(object));
  if(!obj) return 0;
  return obj;
}

object *nil;
object *false;
object *true;
object *symbol_table;
object *quote_symbol;
object *define_symbol;
object *set_symbol;
object *if_symbol;
object *lambda_symbol;

object *the_empty_environment;
object *the_global_environment;

object *cons(object *first, object *rest);
#define car(X) ((X)->data.cons.first)
#define cdr(X) ((X)->data.cons.rest)
#define caar(X) (car(car(X)))
#define caaar(X) (car(car(car(X))))
#define cddr(X) (cdr(cdr(X)))
#define cdddr(X) (cdr(cdr(cdr(X))))
#define cadr(X) (car(cdr(X)))
#define cdar(X) (cdr(car(X)))
#define cdadr(X) (cdr(car(cdr(X))))
#define caddr(X) (car(cdr(cdr(X))))
#define cadddr(X) (car(cdr(cdr(cdr(X)))))
#define caadr(X) (car(car(cdr(X))))

char is_nil(object *obj) {
  return obj->type == NIL;
}

char is_boolean(object *obj) {
  return obj->type == BOOLEAN;
}

char is_false(object *obj) {
  return obj == false;
}

char is_true(object *obj) {
  return !is_false(obj);
}

object *make_symbol(char *value) {
  object *obj;
  object *element;

  // search
  element = symbol_table;
  while(!is_nil(element)) {
    if( strcmp(car(element)->data.symbol.value, value) == 0)
      return car(element);
    element = cdr(element);
  }

  // if not found, create
  obj = alloc_object();
  obj->type = SYMBOL;
  obj->data.symbol.value = (char *) malloc(strlen(value) + 1);
  if(!obj->data.symbol.value) return 0;
  strcpy(obj->data.symbol.value, value);
  symbol_table = cons(obj, symbol_table);
  return obj;
}

char is_symbol(object *obj) {
  return obj->type == SYMBOL;
}

object *make_fixnum(long value) {
  object *obj;

  obj = alloc_object();
  obj->type = FIXNUM;
  obj->data.fixnum.value = value;
  return obj;
}
  
char is_fixnum(object *obj) {
  return obj->type == FIXNUM;
}

object *make_character(char value) {
  object *obj;

  obj = alloc_object();
  obj->type = CHARACTER;
  obj->data.character.value = value;
  return obj;
}

char is_character(object *obj) {
  return obj->type == CHARACTER;
}

object *make_string(char *value) {
  object *obj;

  obj = alloc_object();
  obj->type = STRING;
  obj->data.string.value = (char *) malloc(strlen(value) + 1);
  if (!obj->data.string.value)
    return 0;
  strcpy(obj->data.string.value, value);

  return obj;
}

char is_string(object *obj) {
  return obj->type == STRING;
}

object *cons(object *first, object *rest) {
  object *obj;

  obj = alloc_object();
  obj->type = CONS;
  obj->data.cons.first = first;
  obj->data.cons.rest = rest;

  return obj;
}

char is_cons(object *obj) {
  return obj->type == CONS;
}

object *make_primitive_proc(object *(*fn)(struct object *args)) {
  object *obj;

  obj = alloc_object();
  obj->type = PRIMITIVE_PROC;
  obj->data.primitive_proc.fn = fn;
  return obj;
}

char is_primitive_proc(object *obj) {
  return obj->type == PRIMITIVE_PROC;
}

object *is_null_proc(object *args) {
  return is_nil(car(args)) ? true : false;
}

object *is_boolean_proc(object *args) {
  return is_boolean(car(args)) ? true : false;
}

object *is_symbol_proc(object *args) {
  return is_symbol(car(args)) ? true : false;
}

object *is_integer_proc(object *args) {
  return is_fixnum(car(args)) ? true : false;
}

object *is_char_proc(object *args) {
  return is_character(car(args)) ? true : false;
}

object *is_string_proc(object *args) {
  return is_string(car(args)) ? true : false;
}

object *is_cons_proc(object *args) {
  return is_cons(car(args)) ? true : false;
}

object *is_procedure_proc(object *args) {
  return is_primitive_proc(car(args)) ? true : false;
}

object *char_to_integer_proc(object *args) {
  return make_fixnum(car(args)->data.character.value);
}

object *integer_to_char_proc(object *args) {
  return make_character(car(args)->data.fixnum.value);
}

object *number_to_string_proc(object *args) {
  char buffer[128];

  sprintf(buffer, "%ld", car(args)->data.fixnum.value);
  return make_string(buffer);
}

object *string_to_number_proc(object *args) {
  long num = 0;
  char *cptr = car(args)->data.string.value;

  while(cptr) {
    num = (num / 10) + (*cptr++ - '0');
  }
}

object *symbol_to_string_proc(object *args) {
  return make_string(car(args)->data.symbol.value);
}

object *string_to_symbol_proc(object *args) {
  return make_symbol(car(args)->data.string.value);
}

//#define arithmetic_proc_maker(name,op)          \
//  object *name ## _proc(object *args) {           \
//    long result = 0;                              \
//    while(!is_nil(args)) {                        \
//      result op ## = (car(args))->data.fixnum.value;        \
//      args = cdr(args); \                                   
//    }                                               \
//    return make_fixnum(result); \
//  }
//
//arithmetic_proc_maker(add,+)
//arithmetic_proc_maker

object *add_proc(object *args) {
  long result = 0;

  while (!is_nil(args)) {
    result += (car(args))->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *subtract_proc(object *args) {
  long result = car(args)->data.fixnum.value;
  args = cdr(args);

  while(!is_nil(args)) {
    result -= (car(args))->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *multiply_proc(object *args) {
  long result = 1;

  while(!is_nil(args)) {
    result *= car(args)->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *divide_proc(object *args) {
  long result = car(args)->data.fixnum.value;
  args = cdr(args);

  while(!is_nil(args)) {
    result /= car(args)->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *is_equal_proc(object *args) {
  long value;

  value = car(args)->data.fixnum.value;
  while (!is_nil(args = cdr(args))) {
    if (value != (car(args)->data.fixnum.value))
      return false;
  }
  return true;
}

object *is_less_than_proc(object *args) {
  long previous, next;

  previous = car(args)->data.fixnum.value;
  while( !is_nil(args = cdr(args)) ) {
    next = car(args)->data.fixnum.value;
    if(previous < next)
      previous = next;
    else
      return false;
  }
  return true;
}

object *is_greater_than_proc(object *args) {
  long previous, next;

  previous = car(args)->data.fixnum.value;
  while(!is_nil(args = cdr(args))) {
    next = car(args)->data.fixnum.value;
    if (previous > next)
      previous = next;
    else
      return false;
  }
  return true;
}

object *cons_proc(object *args) {
  return cons(car(args), cadr(args));
}

object *car_proc(object *args) {
  return caar(args);
}

object *cdr_proc(object *args) {
  return cdar(args);
}

object *set_car_proc(object *args) {
  car(car(args)) = cadr(args);
  return cadr(args);
}

object *set_cdr_proc(object *args) {
  cdr(car(args)) = cadr(args);
  return cadr(args);
}

object *list_proc(object *args) {
  return args;
}

object *is_eq_proc(object *args) {
  object *obj1, *obj2;

  obj1 = car(args);
  obj2 = cdr(args);

  if(obj1->type != obj2->type)
    return false;
  switch (obj1->type) {
  case FIXNUM:
    return (obj1->data.fixnum.value ==
            obj2->data.fixnum.value) ? true : false;
    break;
  case CHARACTER:
    return (obj1->data.character.value ==
            obj2->data.character.value) ? true : false;
    break;
  case STRING:
    return (strcmp(obj1->data.string.value,
                   obj2->data.string.value) == 0) ?
      true : false;
    break;
  default:
    return (obj1 == obj2) ? true : false;
    break;
  }
}

object *make_compound_proc(object *params,
                           object *body,
                           object *env) {
  object *obj;

  obj = alloc_object();
  obj->type = COMPOUND_PROC;
  obj->data.compound_proc.parameters = params;
  obj->data.compound_proc.body = body;
  obj->data.compound_proc.env = env;

  return obj;
}

char is_compound_proc(object *obj) {
  return obj->type == COMPOUND_PROC;
}
  
object *enclosing_environment(object *env) {
  return cdr(env);
}

object *first_frame(object *env) {
  return car(env);
}

object *make_frame(object *variables, object *values) {
  return cons(variables, values);
}

object *frame_variables(object *frame) {
  return car(frame);
}

object *frame_values(object *frame) {
  return cdr(frame);
}

void add_binding_to_frame(object *var,
                          object *val,
                          object *frame) {
  car(frame) = cons(var, car(frame) );
  cdr(frame) = cons(val, cdr(frame) );
}

object *extend_environment(object *vars,
                           object *vals,
                           object *base_env) {
  return cons(make_frame(vars, vals), base_env); 
}

object *lookup_variable_value(object *var, object *env) {
  object *frame, *vars, *vals;
  while(!is_nil(env)) {
    frame = first_frame(env);
    vars = frame_variables(frame);
    vals = frame_values(frame);
    while(!is_nil(vars)) {
      if(var == car(vars))
        return car(vals);
      vars = cdr(vars);
      vals = cdr(vals);
    }
    env = enclosing_environment(env);
  }
  fprintf(stderr, "Unbound variable.\n");
  exit(1);
}

void set_variable_value(object *var,
                        object *val,
                        object *env) {
  object *frame, *vars, *vals;
  while(!is_nil(env)) {
    frame = first_frame(env);
    vars = frame_variables(frame);
    vals = frame_values(frame);
    while(!is_nil(vars)) {
      if(var == car(vars)) {
        car(vals) = val;
        return;
      }
      vars = cdr(vars);
      vals = cdr(vals);
    }
    env = enclosing_environment(env);
  }
  fprintf(stderr, "Unbound variable.\n");
  exit(1);
}

void define_variable(object *var,
                     object *val,
                     object *env) {
  object *frame, *vars, *vals;
  frame = first_frame(env);
  vars = frame_variables(frame);
  vals = frame_values(frame);

  while(!is_nil(vars)) {
    if(var == car(vars)) {
      car(vals) = val;
      return;
    }
    vars = cdr(vars);
    vals = cdr(vals);
  }
  add_binding_to_frame(var, val, frame);
}

object *setup_environment() {
  object *initial_env;

  initial_env = extend_environment(nil,
                                   nil,
                                   the_empty_environment);
  return initial_env;
}

#define add_procedure(scheme_name, c_name)      \
  define_variable(make_symbol(scheme_name),     \
                  make_primitive_proc(c_name),  \
                  the_global_environment);

void init() {
  nil = alloc_object();
  nil->type = NIL;
  
  false = alloc_object();
  false->type = BOOLEAN;
  false->data.boolean.value = 0;

  true = alloc_object();
  true->type = BOOLEAN;
  true->data.boolean.value = 1;

  symbol_table = nil;
  quote_symbol = make_symbol("quote");
  define_symbol = make_symbol("define");
  set_symbol = make_symbol("set!");
  if_symbol = make_symbol("if");
  lambda_symbol = make_symbol("lambda");

  the_empty_environment = nil;
  the_global_environment = setup_environment();

  //define_variable(make_symbol("+"),
  //                make_primitive_proc(add_proc),
  //                the_global_environment);
  add_procedure("null?"      , is_null_proc      );
  add_procedure("nil?"       , is_null_proc      );
  add_procedure("boolean?"   , is_boolean_proc   );
  add_procedure("symbol?"    , is_symbol_proc    );
  add_procedure("integer?"   , is_integer_proc   );
  add_procedure("char?"      , is_char_proc      );
  add_procedure("string?"    , is_string_proc    );
  add_procedure("procedure?" , is_procedure_proc );

  add_procedure("char->integer", char_to_integer_proc);
  add_procedure("integer->char", integer_to_char_proc);
  add_procedure("number->string", number_to_string_proc);
  add_procedure("string->number", string_to_number_proc);
  add_procedure("symbol->string", symbol_to_string_proc);
  add_procedure("string->symbol", string_to_symbol_proc);

  add_procedure("+", add_proc);
  add_procedure("-", subtract_proc);
  add_procedure("*", multiply_proc);
  add_procedure("/", divide_proc);
  add_procedure("=", is_equal_proc);
  add_procedure("<", is_less_than_proc);
  add_procedure(">", is_greater_than_proc);

  add_procedure("cons", cons_proc);
  add_procedure("car", car_proc);
  add_procedure("cdr", cdr_proc);
  add_procedure("set-car!", set_car_proc);
  add_procedure("set-cdr!", set_cdr_proc);
  add_procedure("list", list_proc);

  add_procedure("eq?", is_eq_proc);

  
}

/********/
/* read */
/********/

char is_delimiter(int c) {
  return isspace(c) || c == EOF || c == '(' || c == ')' || c == '"' || c == ';';
}

char is_initial(int c) {
  return isalpha(c) || c == '*' || c == '/' || c == '>' ||
    c == '<' || c == '=' || c == '?' || c == '!';
}

int peek(FILE *in) {
  int c;

  c = getc(in);
  ungetc(c, in);
  return c;
}

void eat_whitespace(FILE *in) {
  int c;

  while( (c = getc(in)) != EOF ) {
    if (isspace(c))
      continue;
    else if (c == ';') {
      while( (c = getc(in) != EOF) && (c != '\n') );
      continue;
    }
    ungetc(c, in);
    break;
  }
}

object *read(FILE *in);

object *read_pair(FILE *in) {
  int c;

  object *first_obj;
  object *rest_obj;

  eat_whitespace(in);

  c = getc(in);
  if(c == ')')
    return nil;
  ungetc(c, in);
  
  first_obj = read(in);

  eat_whitespace(in);

  c = getc(in);
  if (c == '.') {
    c = peek(in);
    if (!is_delimiter(c)) {
      fprintf(stderr,"Dot not followed by delimiter.\n");
      exit(1);
    }
    rest_obj = read(in);
    eat_whitespace(in);
    c = getc(in);
    if (c != ')') {
      fprintf(stderr,"Unclosed list.\n");
      exit(1);
    }
    return cons(first_obj, rest_obj);
  }
  else {
    ungetc(c, in);
    rest_obj = read_pair(in);
    return cons(first_obj, rest_obj);
  }
}

object *read(FILE *in) {
  int c;
  int i;
  short sign = 1;
  long num = 0;
  char character;
  char buffer[BUFFER_MAX];

  eat_whitespace(in);

  c = getc(in);

  if(c == EOF)
    return NULL;
  else if(c == '#') {
    c = getc(in);
    switch(c) {
    //read a character
    case '\'':
      character = getc(in);
      if(character == '\\') {
        character = getc(in);
        switch(character) {
        case 'n':
          character = '\n';
          break;
        case 't':
          character = '\t';
          break;
        case 's':
          character = ' ';
          break;
        default:
          fprintf(stderr, "Unrecognized special character '\\%c'\n",character);
          exit(1);
          break;
        }
      }
      if((c = getc(in)) != '\'') {
         fprintf(stderr, "Unexpected character terminator %c\n",c);
         exit(1);
      }
      return make_character(character);
      break;
    // read a boolean
    case 't':
      return true;
      break;
    case 'f':
      return false;
      break;
    default:
      fprintf(stderr, "Unknown boolean literal %c\n",c);
      exit(1);
    }
  }
  // read a fixnum
  else if(isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    if(c == '-')
      sign = -1;
    else
      ungetc(c, in);

    while(isdigit(c = getc(in)))
      num = num * 10 + (c - '0');
    num *= sign;

    if(is_delimiter(c)) {
      ungetc(c, in);
      return make_fixnum(num);
    }
    else {
      fprintf(stderr, "Number not followed by delimiter\n");
      exit(1);
    }
  }
  //read a symbol
  else if (is_initial(c) || ((c == '+' || c == '-') &&
                             is_delimiter(peek(in)))) {
    i = 0;
    while (is_initial(c) || isdigit(c) || c == '+' || c == '-') {
      if (i < BUFFER_MAX - 1) {
        buffer[i++] = c;
      }
      else {
        fprintf(stderr, "Symbol too long; max length is %d.\n", BUFFER_MAX);
        exit(1);
      }
      c = getc(in);
    }
    if( is_delimiter(c) ) {
      buffer[i] = '\0';
      ungetc(c, in);
      return make_symbol(buffer);
    }
    else {
      fprintf(stderr, "Symbol not followed by delimiter; found '%c'.\n", c);
      exit(1);
    }
  }
  //read a string
  else if (c == '"') {
    i = 0;
    while ((c = getc(in)) != '"') {
      if(c == '\\') {
        c = getc(in);
        if(c == 'n')
          c = '\n';
      }
      if(c == EOF) {
        fprintf(stderr,"Non-terminated string literal.\n");
        exit(1);
      }
      // save space for null terminator
      if(i < BUFFER_MAX - 1)
        buffer[i++] = c;
      else {
        fprintf(stderr, "String overflow; maximum string length is %d.\n", BUFFER_MAX);
        exit(1);
      }
    }
    buffer[i] = '\0';
    return make_string(buffer);
  }
  //read a pair
  else if(c == '(') {
    return read_pair(in);
  }
  //read a quoted expression
  else if(c == '\'') {
    return cons(quote_symbol, cons(read(in), nil));
  }
  else {
    fprintf(stderr, "Bad input: unexpected '%c'\n", c);
    exit(1);
  }
}

/********/
/* eval */
/********/

char is_self_evaluating(object *obj) {
  return is_boolean(obj) || is_fixnum(obj) || is_character(obj) || is_string(obj);
}

char is_variable(object *exp) {
  return is_symbol(exp);
}

char is_tagged_list(object *expression, object *tag) {
  object *first;

  if(is_cons(expression)) {
    first = car(expression);
    return is_symbol(first) && (first == tag);
  }
  return 0;
}

char is_quoted(object *expression) {
  return is_tagged_list(expression, quote_symbol);
}

object *text_of_quotation(object *expression) {
  return cadr(expression);
}

char is_assignment(object *exp) {
  return is_tagged_list(exp, set_symbol);
}

object *assignment_variable(object *exp) {
  //return car(cdr(exp));
  return cadr(exp);
}

object *assignment_value(object *exp) {
  //return car(cdr(cdr(exp)));
  return caddr(exp);
}

char is_definition(object *exp) {
  return is_tagged_list(exp, define_symbol);
}

object *definition_variable(object *exp) {
  //return cadr(exp);
  if (is_symbol(cadr(exp)))
    return cadr(exp);
  else
    return caadr(exp);
}

object *make_lambda(object *params, object *body);

object *definition_value(object *exp) {
  //return caddr(exp);
  if(is_symbol(cadr(exp)))
    return caddr(exp);
  //cdadr: formal parameters
  //cddr : body
  else
    return make_lambda(cdadr(exp), cddr(exp));
}

char is_if(object *exp) {
  return is_tagged_list(exp, if_symbol);
}

object *if_predicate(object *exp) {
  return cadr(exp);
}

object *if_consequent(object *exp) {
  return caddr(exp);
}

object *if_alternative(object *exp) {
  if (is_nil(cdddr(exp)))
    return false;
  return cadddr(exp);
}

object *make_lambda(object *params, object *body) {
  return cons(lambda_symbol, cons(params, body));
}

char is_lambda(object *exp) {
  return is_tagged_list(exp, lambda_symbol);
}

object *lambda_parameters(object *exp) {
  return cadr(exp);
}

object *lambda_body(object *exp) {
  return cddr(exp);
}

char is_last_exp(object *exps) {
  return is_nil(cdr(exps));
}

char is_application(object *exp) {
  return is_cons(exp);
}

object *operator(object *exp) {
  return car(exp);
}

object *operands(object *exp) {
  return cdr(exp);
}

char is_no_operands(object *ops) {
  return is_nil(ops);
}

object *first_operand(object *ops) {
  return car(ops);
}

object *rest_operands(object *ops) {
  return cdr(ops);
}

object *eval(object *exp, object *env);
object *eval_sequence(object *exps, object *env) {
  while(!is_last_exp(exps)) {
    eval(car(exps), env);
    exps = cdr(exps);
  }
  return eval(car(exps), env);
}

object *list_of_values(object *exps, object *env) {
  if (is_no_operands(exps)) {
    return nil;
  }
  else {
    return cons(eval(first_operand(exps), env),
                list_of_values(rest_operands(exps), env));
  }
}

object *apply(object *proc, object *args) {
  object *exp;
  if (is_primitive_proc(proc))
    return (proc->data.primitive_proc.fn)(args);
  else if (is_compound_proc(proc)) {
    exp = proc->data.compound_proc.body;
    //while (!is_last_exp(exp)) {
    //  eval(car(exp), env);
    //  exp = cdr(exp);
    //}
    //exp = car(exp);
    return eval_sequence(exp,
                         extend_environment(
                           proc->data.compound_proc.parameters,
                           args,
                           proc->data.compound_proc.env));
  }
  else {
    fprintf(stderr, "Unknown procedure type\n");
    exit(1);
  }
}

object *eval_assignment(object *exp, object *env) {
  set_variable_value(assignment_variable(exp),
                     eval(assignment_value(exp),env),
                     env);
  //return ok_symbol; //vanilla scheme
  return assignment_variable(exp);
}

object *eval_definition(object *exp, object *env) {
  define_variable(definition_variable(exp),
                  eval(definition_value(exp), env),
                  env);
  //return ok_symbol; //vanilla scheme
  return definition_variable(exp);
}

object *eval(object *exp, object *env) {
  object *proc, *args;
  while(1) {
    if(is_self_evaluating(exp)) {
      return exp;
    }
    else if (is_variable(exp)) {
      return lookup_variable_value(exp, env);
    }
    else if (is_quoted(exp)) {
      return text_of_quotation(exp);
    }
    else if (is_assignment(exp)) {
      return eval_assignment(exp, env);
    }
    else if (is_definition(exp)) {
      return eval_definition(exp,env);
    }
    else if (is_if(exp)) {
      exp = is_true(eval(if_predicate(exp), env)) ? if_consequent(exp) : if_alternative(exp);
    }
    else if (is_lambda(exp)) {
      return make_compound_proc(lambda_parameters(exp),
                                lambda_body(exp),
                                env);
    }
    else if (is_application(exp)) {
      proc = eval(operator(exp), env);
      args = list_of_values(operands(exp), env);
      return apply(proc, args);
    }
    else {
      fprintf(stderr,
              "Cannot eval unknown expression type.\n");
      exit(1);
    }
  }
  fprintf(stderr,"Eval illegal state.\n");
  exit(1);
}

/*********/
/* print */
/*********/

void write(object *obj);

void write_pair(object *cons) {
  write(car(cons));
  if((cdr(cons))->type == CONS) {
    printf(" ");
    write_pair(cdr(cons));
  }
  else if ( (cdr(cons))->type == NIL)
    return;
  else {
    printf(" . ");
    write(cdr(cons));
  }
}

void write(object *obj) {
  switch(obj->type) {
  case NIL:
    printf("()");
    break;
  case BOOLEAN:
    printf("#%c", is_false(obj) ? 'f' : 't');
    break;
  case SYMBOL:
    printf("%s",obj->data.symbol.value);
    break;
  case FIXNUM:
    printf("%ld",obj->data.fixnum.value);
    break;
  case CHARACTER:
    printf("#'%c'",obj->data.character.value);
    break;
  case STRING:
    printf("\"%s\"",obj->data.string.value);
    break;
  case CONS:
    printf("(");
    write_pair(obj);
    printf(")");
    break;
  case PRIMITIVE_PROC:
  case COMPOUND_PROC:
    printf("#<procedure>");
    break;
  default:
    fprintf(stderr, "Cannot write unknown type.\n");
    exit(1);
  }
  //fflush(stdout);
}

/********/
/* repl */
/********/

void read_eval_file(FILE* in) {
  object *obj;
  while((obj = read(in)) != NULL) {
    eval(obj, the_global_environment);
    //write(eval(obj, the_global_environment));
    //printf("\n");
  }
}

void read_eval_print_file(FILE *in) {
  object *obj;
  while((obj = read(in)) != NULL) {
    //eval(obj, the_global_environment);
    write(eval(obj, the_global_environment));
    printf("\n");
  }
}

void repl() {
  printf("C-c to exit.\n");
  while(1) {
    printf("=> ");
    write(eval(read(stdin), the_global_environment));
    printf("\n");
  }
}

int main() {
  char bootstrap_code_fname[128] = "bootstrap.l";
  FILE *bootstrap_code;
  //object *obj;
  printf("Iota-Bootstrap.\n");
  
  printf("Initializing core...\n");
  init();
  
  printf("Bootstrapping iota...\n");
  bootstrap_code = fopen(bootstrap_code_fname,"r");
  read_eval_print_file(bootstrap_code);
  fclose(bootstrap_code);
  
  //while (1) {
  //  printf("=> ");
  //  write(eval(read(stdin), the_global_environment));
  //  printf("\n");
  //}

  repl(stdin);

  return 0;
}
