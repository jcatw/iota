#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "iota-bootstrap.h"

#ifndef BUFFER_MAX
#define BUFFER_MAX 1024
#endif

#ifndef NDEBUG
#define DEBUG 1
#endif

/************/
/* language */
/************/

typedef struct object {
  object_type type;
  char mark;
  #ifdef DEBUG
  int gc_count;
  int alloc_count;
  #endif
  union {
    struct {
      char *value;
    } symbol;
    struct {
      char *value;
    } keyword;
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
      struct object * (*fn)(struct object *args, struct object *env);
    } primitive_proc;
    struct {
      struct object *parameters;
      struct object *body;
      struct object *env;
    } compound_proc;
    struct {
      struct object *parameters;
      struct object *body;
      struct object *env;
    } macro;
    struct {
      directiontype directiontype;
      FILE* fp;
    } stream;
  } data;
  struct object *next;
} object;

typedef struct object_stack {
  object **obj;
  struct object_stack *next;
} object_stack;

//allocate a persistent object (ie can not be gc'd)
object *alloc_persistent_object() {
  object *obj;
  obj = (object *) malloc(sizeof(object));
  if(!obj) return 0;
  obj->mark = 1;
  obj->next = NULL;
  return obj;
}

//allocate an object off the heap
//(heap objects can be gc'd)
object *alloc_object() {
  object *free_object;
  object *active_object_head;

  // get a free object
  free_object = get_free_object();
  if(free_object == NULL) {
    // if none left, gc and try again
    gc();
    free_object = get_free_object();
    // if still none left, we are out of memory
    if(free_object == NULL) {
      error("Out of memory.");
    }
  }
  #ifdef DEBUG
  free_object->alloc_count++;
  #endif
  // the free object is now active (no longer free)
  // put it at the head of the active list
  free_object->next = active_list;
  active_list = free_object;
  return free_object;
}

#define N_ROOT_OBJECTS 8

void gc() {
  int i;
  object *root_objects[N_ROOT_OBJECTS]= {nil,
                                         the_global_environment,
                                         the_empty_environment,
                                         symbol_table,
                                         keyword_table,
                                         stdin_stream,
                                         stdout_stream,
                                         eof_object};
  object *iterator;
  object *next;
  object_stack *stack;
  object *old_free_list;
  object *free_ptr;
  object *prev;
  int n_objects_marked = 0;
  int n_objects_swept = 0;
  #ifdef DEBUG
  fprintf(stderr,"gc...\n");
  fprintf(stderr,"mark...\n");
  #endif
  // mark
  for(i=0; i<N_ROOT_OBJECTS; i++) {
    n_objects_marked += mark(root_objects[i]);
  }
  stack = stack_roots;
  while(stack != NULL) {
    n_objects_marked += mark(*(stack->obj));
    stack = stack->next;
  }
  #ifdef DEBUG
  fprintf(stderr,"%d items marked.\n",n_objects_marked);
  fprintf(stderr,"sweep...\n");
  #endif
  //sweep
  iterator = active_list;
  prev = NULL;
  while(!(iterator == NULL)) {
    if(!iterator->mark) {
      //iterator->mark = 0;
      n_objects_swept++;
      #ifdef DEBUG
      iterator->gc_count++;
      #endif
      if(is_string(iterator))
        free(iterator->data.string.value);
      else if(is_symbol(iterator))
        free(iterator->data.symbol.value);
      else if(is_keyword(iterator))
        free(iterator->data.keyword.value);
      next = iterator->next;
      old_free_list = free_list;
      free_list = iterator;
      if(prev != NULL)
        prev->next = next;
      free_list->next = old_free_list;
      iterator = next;
    }
    else {
      iterator->mark = 0;
      prev = iterator;
      iterator = iterator->next;
    }
  }
  #ifdef DEBUG
  fprintf(stderr,"%d items swept.\n",n_objects_swept);
  assert( (n_objects_marked + n_objects_swept) == N_HEAP);
  #endif
}

int mark(object *obj) {
  //int count = 1 - obj->mark;
  int count = 1;
  if(obj->mark == 1)
    return 0;
  obj->mark = 1;
  switch(obj->type) {
  case CONS:
    return count + mark(car(obj)) + mark(cdr(obj));
    break;
  //case PRIMITIVE_PROC:
  //  return count + mark(obj->data.primitive_proc.fn);
  //  break;
  case COMPOUND_PROC:
    return count + mark(obj->data.compound_proc.parameters) +
      mark(obj->data.compound_proc.body) +
      mark(obj->data.compound_proc.env);
    break;
  case MACRO:
    return count + mark(obj->data.macro.parameters) +
      mark(obj->data.macro.body) +
      mark(obj->data.macro.env);
    break;
  default:
    return count;
    break;
  }
}

void error(char *msg) {
  fprintf(stderr,"%s\n",msg);
  exit(1);
}
  
char is_nil(object *obj) {
  return obj->type == NIL;
}

//allocs
object *make_symbol(char *value) {
  object *obj = nil;
  object *element = nil;

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
  if(!obj->data.symbol.value) {
    return 0;
  }
  strcpy(obj->data.symbol.value, value);
  push_stack_root(&obj);
  symbol_table = cons(obj, symbol_table);
  pop_stack_root();
  return obj;
}

//allocs
object *make_keyword(char *value) {
  object *obj = nil;
  object *element = nil;

  // search
  element = keyword_table;
  while(!is_nil(element)) {
    if (strcmp(car(element)->data.keyword.value, value) == 0)
      return car(element);
    element = cdr(element);
  }

  // if not found, create
  obj = alloc_object();
  obj->type = KEYWORD;
  obj->data.keyword.value = (char *) malloc(strlen(value) + 1);
  if(!obj->data.keyword.value)
    return 0;
  strcpy(obj->data.keyword.value, value);
  push_stack_root(&obj);
  keyword_table = cons(obj, keyword_table);
  pop_stack_root();
  return obj;
}

char is_symbol(object *obj) {
  return obj->type == SYMBOL;
}

char is_keyword(object *obj) {
  return obj->type == KEYWORD;
}

//allocs
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

//allocs
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

//allocs
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

//allocs
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

char is_list(object *obj) {
  return obj->type == CONS || obj->type == NIL;
}

char is_atom(object *obj) {
  return !is_list(obj);
}

char is_stream(object *obj) {
  return obj->type == STREAM;
}

char is_output_stream(object *obj) {
  return is_stream(obj) && obj->data.stream.directiontype == OUTPUT;
}

char is_input_stream(object *obj) {
  return is_stream(obj) && obj->data.stream.directiontype == INPUT;
}

//allocs
object *make_file_stream(char* stream_name, directiontype direction) {
  object *obj;
  int fd;

  obj = alloc_object();
  obj->type = STREAM;

  if(strcmp(stream_name, "stdin") == 0 ) {
    obj->data.stream.fp = stdin;
    obj->data.stream.directiontype = INPUT;
    return obj;
  }
  if (strcmp(stream_name, "stdout") == 0) {
    obj->data.stream.fp = stdout;
    obj->data.stream.directiontype = OUTPUT;
    return obj;
  }
  if (direction == INPUT) {
    obj->data.stream.directiontype = INPUT;
    fd = open(stream_name, O_RDONLY | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR);
    obj->data.stream.fp = fdopen(fd, "r");
  }
  else {
    obj->data.stream.directiontype = OUTPUT;
    fd = open(stream_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    obj->data.stream.fp = fdopen(fd, "w");
  }
  if(!fd || !obj->data.stream.fp) {
    error("Could not open file stream.");
  }
  // turn off buffering
  setvbuf(obj->data.stream.fp, NULL, _IONBF, 0);
  return obj;
}


void close_stream(object *stream) {
  assert( is_stream(stream) );
  fclose(stream->data.stream.fp);
}

// get length of list
// linear time! use sparingly.
long len(object *obj) {
  int l = 0;
  if(!obj->type == CONS)
    return 1;
  else {
    while(!is_nil(obj)) {
      l++;
      obj = cdr(obj);
    }
    return l;
  }
}

//allocs
object *make_primitive_proc(object *(*fn)(struct object *args, struct object *env)) {
  object *obj;

  obj = alloc_object();
  obj->type = PRIMITIVE_PROC;
  obj->data.primitive_proc.fn = fn;
  return obj;
}

char is_primitive_proc(object *obj) {
  return obj->type == PRIMITIVE_PROC;
}

object *error_proc(object *args, object *env) {
  assert( is_list(args) );
  object *msg;
  msg = car(args);
  assert( is_string(msg) );
  error(msg->data.string.value);
  return nil;
}

object *is_null_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_nil(car(args)) ? t_symbol : nil;
}

object *is_list_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_list(car(args)) ? t_symbol : nil;
}

object *is_atom_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_atom(car(args)) ? t_symbol : nil;
}

object *is_symbol_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_symbol(car(args)) ? t_symbol : nil;
}

object *is_keyword_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_keyword(car(args)) ? t_symbol : nil;
}

object *is_integer_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_fixnum(car(args)) ? t_symbol : nil;
}

object *is_char_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_character(car(args)) ? t_symbol : nil;
}

object *is_string_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_string(car(args)) ? t_symbol : nil;
}

object *is_cons_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_cons(car(args)) ? t_symbol : nil;
}

object *is_procedure_proc(object *args, object *env) {
  assert( is_list(args) );
  return is_primitive_proc(car(args)) ? t_symbol : nil;
}

char is_tagged_list(object *expression, object *tag) {
  object *first;

  if(is_cons(expression)) {
    first = car(expression);
    return is_symbol(first) && (first == tag);
  }
  return 0;
}

object *is_tagged_list_proc(object *args, object *env) {
  assert( is_list(args) );
  object *exp, *tag;
  exp = car(args);
  tag = cadr(args);

  return is_tagged_list(exp, tag) ? t_symbol : nil;
}

object *char_to_integer_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_character(car(args)) );
  return make_fixnum(car(args)->data.character.value);
}

object *integer_to_char_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_fixnum(car(args)) );
  return make_character(car(args)->data.fixnum.value);
}

object *number_to_string_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_fixnum(car(args)) );
  char buffer[128];

  sprintf(buffer, "%ld", car(args)->data.fixnum.value);
  return make_string(buffer);
}

object *string_to_number_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_string(car(args)) );
  long num = 0;
  char *cptr = car(args)->data.string.value;

  while(cptr) {
    num = (num / 10) + (*cptr++ - '0');
  }
}

object *concat_proc(object *args, object *env) {
  assert( is_list(args) );
  char new_string[BUFFER_MAX];
  object *strobj1, *strobj2;
  strobj1 = car(args);
  strobj2 = cadr(args);

  strcpy(new_string, strobj1->data.string.value);
  strcat(new_string, strobj2->data.string.value);
  return make_string(new_string);
}

object *symbol_to_string_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_symbol(car(args)) );
  return make_string(car(args)->data.symbol.value);
}

object *string_to_symbol_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_string(car(args)) );
  return make_symbol(car(args)->data.string.value);
}

object *add_proc(object *args, object *env) {
  assert( is_list(args) );
  long result = 0;

  while (!is_nil(args)) {
    assert( is_fixnum(car(args)) );
    result += (car(args))->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *subtract_proc(object *args, object *env) {
  assert( is_list(args) );
  long result = car(args)->data.fixnum.value;
  args = cdr(args);

  while(!is_nil(args)) {
    assert( is_fixnum(car(args)) );
    result -= (car(args))->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *multiply_proc(object *args, object *env) {
  assert( is_list(args) );
  long result = 1;

  while(!is_nil(args)) {
    assert( is_fixnum(car(args)) );
    result *= car(args)->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *divide_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_fixnum(car(args)) );
  long result = car(args)->data.fixnum.value;
  args = cdr(args);

  while(!is_nil(args)) {
    assert( is_fixnum(car(args)) );
    result /= car(args)->data.fixnum.value;
    args = cdr(args);
  }
  return make_fixnum(result);
}

object *is_equal_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_fixnum(car(args)) );
  long value;

  value = car(args)->data.fixnum.value;
  while (!is_nil(args = cdr(args))) {
    assert( is_fixnum(car(args)) );
    if (value != (car(args)->data.fixnum.value))
      return nil;
  }
  return t_symbol;
}

object *is_less_than_proc(object *args, object *env) {
  assert( is_list(args) );
  long previous, next;

  previous = car(args)->data.fixnum.value;
  while( !is_nil(args = cdr(args)) ) {
    assert( is_fixnum(car(args)) );
    next = car(args)->data.fixnum.value;
    if(previous < next)
      previous = next;
    else
      return nil;
  }
  return t_symbol;
}

object *is_greater_than_proc(object *args, object *env) {
  assert( is_list(args) );
  long previous, next;

  previous = car(args)->data.fixnum.value;
  while(!is_nil(args = cdr(args))) {
    assert( is_fixnum(car(args)) );
    next = car(args)->data.fixnum.value;
    if (previous > next)
      previous = next;
    else
      return nil;
  }
  return t_symbol;
}

object *cons_proc(object *args, object *env) {
  assert( is_list(args) );
  return cons(car(args), cadr(args));
}

object *car_proc(object *args, object *env) {
  assert( is_list(args) );
  return caar(args);
}

object *cdr_proc(object *args, object *env) {
  assert( is_list(args) );
  return cdar(args);
}

object *set_car_proc(object *args, object *env) {
  assert( is_list(args) );
  car(car(args)) = cadr(args);
  return cadr(args);
}

object *set_cdr_proc(object *args, object *env) {
  assert( is_list(args) );
  cdr(car(args)) = cadr(args);
  return cadr(args);
}

object *list_proc(object *args, object *env) {
  assert( is_list(args) );
  return args;
}

object *len_proc(object *args, object *env) {
  assert( is_list(args) );
  return make_fixnum(len(car(args)));
}

char is_eq(object *obj1, object *obj2) {
  if(obj1->type != obj2->type)
    return 0;
  switch (obj1->type) {
  case FIXNUM:
    return (obj1->data.fixnum.value ==
            obj2->data.fixnum.value);
    break;
  case CHARACTER:
    return (obj1->data.character.value ==
            obj2->data.character.value);
    break;
  case STRING:
    return (strcmp(obj1->data.string.value,
                   obj2->data.string.value) == 0);
    break;
  default:
    return (obj1 == obj2);
    break;
  }
}

object *is_eq_proc(object *args, object *env) {
  assert( is_list(args) );
  object *obj1, *obj2;

  obj1 = car(args);
  obj2 = cadr(args);

  return is_eq(obj1, obj2) ? t_symbol : nil;
}

object *reverse(object *head) {
  assert( is_list(head) );
  object *new_head = nil;
  push_stack_root(&new_head);
  while(!is_nil(head)) {
    new_head = cons(car(head), new_head);
    head = cdr(head);
  }
  pop_stack_root();
  return new_head;
}

object *reverse_proc(object *args, object *env) {
  assert( is_list(args) );
  assert( is_list(car(args)) );
  return reverse(car(args));
}

object *make_file_stream_proc(object *args, object *env) {
  assert( is_list(args) );
  object *name, *dtype;
  name = car(args);
  dtype = cadr(args);
  assert( is_string(name) );
  assert( is_keyword(dtype) );
  
  return make_file_stream(name->data.string.value,
                          is_eq(dtype, output_keyword) ? OUTPUT : INPUT);
}

object *close_stream_proc(object *args, object *env) {
  assert( is_list(args) );
  object *stream;

  stream = car(args);
  assert(is_stream(stream));
  close_stream(stream);

  return t_symbol;
}

//allocs
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
  assert( is_list(env) );
  return cdr(env);
}

object *first_frame(object *env) {
  assert( is_list(env) );
  return car(env);
}

object *make_frame(object *variables, object *values) {
  return cons(variables, values);
}

object *frame_variables(object *frame) {
  assert( is_list(frame) );
  return car(frame);
}

object *frame_values(object *frame) {
  assert( is_list(frame) );
  return cdr(frame);
}

void add_binding_to_frame(object *var,
                          object *val,
                          object *frame) {
  object *result = nil;
  push_stack_root(&result);
  result = cons(var, car(frame) );
  car(frame) = result;
  result = cons(val, cdr(frame) );  
  cdr(frame) = result;
  pop_stack_root();
}

object *extend_environment(object *vars,
                           object *vals,
                           object *base_env) {
  assert( is_list(base_env) );
  object *result = nil;
  push_stack_root(&result);
  result = make_frame(vars, vals);
  result = cons(result, base_env);
  pop_stack_root();
  return result; 
}

object *lookup_variable_value(object *var, object *env) {
  assert( is_list(env) );
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
  error("Unbound variable.");
}

void set_variable_value(object *var,
                        object *val,
                        object *env) {
  assert( is_list(env)  );
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
  error("Unbound variable.");
}

void define_variable(object *var,
                     object *val,
                     object *env) {
  assert( is_list(env) );
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

object *global_environment_proc(object *args, object *env) {
  return the_global_environment;
}

object *alloc_initial_objects(int n_objects) {
  int i;
  object *prev_object;
  object *current_object;

  //allocate space for n_objects objects and chain them together
  object *root_object = (object *) malloc(n_objects * sizeof(object));
  if(!root_object) return 0;
  root_object->mark = 0;
  #ifdef DEBUG
  root_object->gc_count = 0;
  root_object->alloc_count = 0;
  #endif
  
  current_object = root_object;
  for(i=1; i<n_objects; i++) {
    prev_object = current_object;
    current_object = prev_object + 1;
    current_object->mark = 0;
    #ifdef DEBUG
    current_object->gc_count = 0;
    current_object->alloc_count = 0;
    #endif
    prev_object->next = current_object;
  }
  current_object->next = NULL;
  return root_object;
}

object *get_free_object() {
  object *free_object = free_list;
  if(free_object == NULL)
    return NULL;
  free_list = free_list->next;
  return free_object;
}

void push_stack_root(object **root) {
  object_stack *old_stack_head;

  old_stack_head = stack_roots;
  stack_roots = (object_stack*) malloc( sizeof(object_stack) );
  stack_roots->obj = root;
  stack_roots->next = old_stack_head;
}

object **pop_stack_root() {
  object_stack *stack_head;
  object **stack_head_object_ptr;

  stack_head = stack_roots;
  if (stack_roots == NULL)
    error("No stack frame to pop.");
  stack_roots = stack_roots->next;
  stack_head_object_ptr = stack_head->obj;
  free(stack_head);
  return stack_head_object_ptr;
}

void init() {
  free_list = alloc_initial_objects(N_HEAP);
  
  //nil = alloc_persistent_object();
  nil = alloc_object();
  nil->type = NIL;

  symbol_table = nil;
  keyword_table = nil;
  
  t_symbol = make_symbol("t");
  quote_symbol = make_symbol("quote");
  backquote_symbol = make_symbol("backquote");
  comma_symbol = make_symbol("comma");
  pipe_symbol = make_symbol("pipe");
  comma_at_symbol = make_symbol("comma-at");
  define_symbol = make_symbol("define");
  set_symbol = make_symbol("set!");
  if_symbol = make_symbol("if");
  cond_symbol = make_symbol("cond");
  else_symbol = make_symbol("else");
  lambda_symbol = make_symbol("lambda");
  let_symbol = make_symbol("let");
  begin_symbol = make_symbol("begin");
  macro_symbol = make_symbol("macro");
  rest_keyword = make_keyword(":rest");
  output_keyword = make_keyword(":output");
  input_keyword = make_keyword(":input");

  the_empty_environment = nil;
  the_global_environment = setup_environment();

  define_variable(make_symbol("nil"),
                  nil,
                  the_global_environment);
  define_variable(t_symbol,
                  t_symbol,
                  the_global_environment);

  eof_object = make_character(EOF);
  stdin_stream = make_file_stream("stdin", INPUT);
  stdout_stream = make_file_stream("stdout", OUTPUT);
  stdin_symbol = make_symbol("*stdin*");
  stdout_symbol = make_symbol("*stdout*");
  define_variable(stdin_symbol,
                  stdin_stream,
                  the_global_environment);
  define_variable(stdout_symbol,
                  stdout_stream,
                  the_global_environment);

  add_procedure("error"        , error_proc          );
  add_procedure("null?"        , is_null_proc        );
  add_procedure("nil?"         , is_null_proc        );
  add_procedure("symbol?"      , is_symbol_proc      );
  add_procedure("keyword?"     , is_keyword_proc     );
  add_procedure("integer?"     , is_integer_proc     );
  add_procedure("char?"        , is_char_proc        );
  add_procedure("string?"      , is_string_proc      );
  add_procedure("procedure?"   , is_procedure_proc   );
  add_procedure("list?"        , is_list_proc        );
  add_procedure("atom?"        , is_atom_proc        );
  add_procedure("tagged-list?" , is_tagged_list_proc );

  add_procedure("char->integer"  , char_to_integer_proc  );
  add_procedure("integer->char"  , integer_to_char_proc  );
  add_procedure("number->string" , number_to_string_proc );
  add_procedure("string->number" , string_to_number_proc );
  add_procedure("symbol->string" , symbol_to_string_proc );
  add_procedure("string->symbol" , string_to_symbol_proc );

  add_procedure("strcat", concat_proc);

  add_procedure("+" , add_proc             );
  add_procedure("-" , subtract_proc        );
  add_procedure("*" , multiply_proc        );
  add_procedure("/" , divide_proc          );
  add_procedure("=" , is_equal_proc        );
  add_procedure("<" , is_less_than_proc    );
  add_procedure(">" , is_greater_than_proc );

  add_procedure("cons"     , cons_proc    );
  add_procedure("car"      , car_proc     );
  add_procedure("cdr"      , cdr_proc     );
  add_procedure("set-car!" , set_car_proc );
  add_procedure("set-cdr!" , set_cdr_proc );
  add_procedure("list"     , list_proc    );
  add_procedure("len"      , len_proc     );
  add_procedure("reverse"  , reverse_proc );

  add_procedure("eq?", is_eq_proc);
  
  add_procedure("macroexpand" , macroexpand_proc        );
  add_procedure("apply"       , apply_proc              );
  add_procedure("eval"        , eval_proc               );
  add_procedure("read"        , read_proc               );
  add_procedure("write"       , write_proc              );
  add_procedure("global-env"  , global_environment_proc );

  add_procedure("make-file-stream"   , make_file_stream_proc   );
  add_procedure("close-stream"       , close_stream_proc       );
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

object *read_pair(object *in_stream, object *env) {
  int i;
  FILE *in;
  int c;

  object *first_obj = nil;
  object *rest_obj = nil;
  object *result = nil;

  if(in_stream == stdin_stream)
    in_stream = eval(stdin_symbol, env);
  in = in_stream->data.stream.fp;

  eat_whitespace(in);

  c = getc(in);
  if(c == ')')
    return nil;
  ungetc(c, in);

  push_stack_root(&first_obj);
  first_obj = read(in_stream, env);
  
  eat_whitespace(in);

  c = getc(in);
  if (c == '.') {
    c = peek(in);
    if (!is_delimiter(c)) {
      error("Dot not followed by delimiter");
    }
    push_stack_root(&rest_obj);
    rest_obj = read(in_stream, env);
    eat_whitespace(in);
    c = getc(in);
    if (c != ')') {
      error("Unclosed list.");
    }
    push_stack_root(&result);
    result = cons(first_obj, rest_obj);
    for(i=0;i<3;i++) {
      pop_stack_root();
    }  
    return result;
  }
  else {
    ungetc(c, in);
    push_stack_root(&rest_obj);
    rest_obj = read_pair(in_stream, env);
    push_stack_root(&result);
    result = cons(first_obj, rest_obj);
    for(i=0;i<3;i++) {
      pop_stack_root();
    }
    return result;
  }
}

object *read(object *in_stream, object *env) {
  FILE *in;
  int c;
  int i;
  short sign = 1;
  long num = 0;
  char character;
  char buffer[BUFFER_MAX];
  char error_msg[BUFFER_MAX];
  object *result = nil;

  if(in_stream == stdin_stream)
    in_stream = eval(stdin_symbol, env);
  
  in = in_stream->data.stream.fp;

  eat_whitespace(in);

  c = getc(in);

  if(c == EOF)
    return eof_object;
  else if(c == '#') {
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
        sprintf(error_msg,"Unrecognized special character '\\%c'",character);
        error(error_msg);
        break;
      }
    }
    return make_character(character);
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
      error("Number not followed by delimiter");
    }
  }
  //read a keyword
  else if(c == ':') {
    i = 0;
    while (c == ':' || is_initial(c) || isdigit(c) || c == '+' || c == '-') {
      if (i < BUFFER_MAX - 1) {
        buffer[i++] = c;
      }
      else {
        sprintf(error_msg,"Keyword too long; max length is %d.", BUFFER_MAX);
        error(error_msg);
      }
      c = getc(in);
    }
    if( is_delimiter(c) ) {
      buffer[i] = '\0';
      ungetc(c, in);
      return make_keyword(buffer);
    }
    else {
      sprintf(error_msg, "Keyword not followed by delimiter; found '%c'.", c);
      error(error_msg);
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
        sprintf(error_msg, "Symbol too long; max length is %d.", BUFFER_MAX);
        error(error_msg);
      }
      c = getc(in);
    }
    if( is_delimiter(c) ) {
      buffer[i] = '\0';
      ungetc(c, in);
      return make_symbol(buffer);
    }
    else {
      sprintf(error_msg, "Symbol not followed by delimiter; found '%c'.", c);
      error(error_msg);
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
        sprintf(error_msg,"Non-terminated string literal.");
        error(error_msg);
      }
      // save space for null terminator
      if(i < BUFFER_MAX - 1)
        buffer[i++] = c;
      else {
        sprintf(error_msg, "String overflow; maximum string length is %d.", BUFFER_MAX);
        error(error_msg);
      }
    }
    buffer[i] = '\0';
    return make_string(buffer);
  }
  //read a pair
  else if(c == '(') {
    return read_pair(in_stream, env);
  }
  //read a quoted expression
  else if(c == '\'') {
    push_stack_root(&result);
    result = cons(read(in_stream, env), nil);
    result = cons(quote_symbol, result);
    pop_stack_root();
    return result;
  }
  //read a backquoted expression
  else if(c == '`') {
    push_stack_root(&result);
    result = cons(read(in_stream, env), nil);
    result = cons(backquote_symbol, result);
    pop_stack_root();
    return result;
  }
  // read an escaped (comma'd) expression
  else if(c == ',') {
    if(peek(in) == '@') {
      c = getc(in);
      push_stack_root(&result);
      result = cons(read(in_stream,env), nil);
      result = cons(comma_at_symbol, result);
      pop_stack_root();
      return result;
    }
    else {
      push_stack_root(&result);
      result = cons(read(in_stream,env), nil);
      result = cons(comma_symbol, result);
      pop_stack_root();
      return result;
    }
  }
  // read a evaler'd (|'d) expression
  else if(c == '|') {
    push_stack_root(&result);
    result = cons(read(in_stream,env), nil);
    result = cons(pipe_symbol, result);
    pop_stack_root();
    return result;
    //return eval( read(in_stream, env), env);
  }
  else {
    sprintf(error_msg, "Bad input: unexpected '%c'", c);
    error(error_msg);
  }
}

object *read_proc(object *args, object *env) {
  assert( is_list(args) );
  //object *env;
  object *ins;
  //env = car(args);
  if( is_nil(args) ) {
    //ins = eval(make_symbol("*stdin*"), env);
    ins = eval(stdin_symbol, env);
  }
  else {
    ins = car(args);
  }

  assert( is_stream(ins) );
  return read(ins,env);
}
  
/********/
/* eval */
/********/

char is_self_evaluating(object *obj) {
  return is_nil(obj) || is_keyword(obj) || is_fixnum(obj) || is_character(obj) || is_string(obj);
}

char is_variable(object *exp) {
  return is_symbol(exp);
}

char is_quoted(object *expression) {
  return is_tagged_list(expression, quote_symbol);
}

object *text_of_quotation(object *expression) {
  assert( is_list(expression) );
  return cadr(expression);
}

char is_assignment(object *exp) {
  return is_tagged_list(exp, set_symbol);
}

object *assignment_variable(object *exp) {
  assert( is_list(exp) );
  return cadr(exp);
}

object *assignment_value(object *exp) {
  assert( is_list(exp) );
  return caddr(exp);
}

char is_definition(object *exp) {
  return is_tagged_list(exp, define_symbol);
}

object *definition_variable(object *exp) {
  assert( is_list(exp) );
  if (is_symbol(cadr(exp)))
    return cadr(exp);
  else
    return caadr(exp);
}

object *definition_value(object *exp) {
  assert( is_list(exp) );
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
  assert( is_list(exp) );
  return cadr(exp);
}

object *if_consequent(object *exp) {
  assert( is_list(exp) );
  return caddr(exp);
}

object *if_alternative(object *exp) {
  assert( is_list(exp) );
  if (is_nil(cdddr(exp)))
    return nil;
  return cadddr(exp);
}

object *make_lambda(object *params, object *body) {
  object *result = nil;
  push_stack_root(&result);
  result = cons(params, body);
  result = cons(lambda_symbol, result);
  pop_stack_root();
  return result;
}

char is_lambda(object *exp) {
  return is_tagged_list(exp, lambda_symbol);
}

object *lambda_parameters(object *exp) {
  assert( is_list(exp) );
  return cadr(exp);
}

object *lambda_body(object *exp) {
  assert( is_list(exp) );
  return cddr(exp);
}

char is_macro (object *obj) {
  return obj->type == MACRO;
}

char is_macro_def(object *exp) {
  return is_tagged_list(exp, macro_symbol);
}
//allocs
object *make_macro(object *params,
                   object *body,
                   object *env) {
  object *obj;

  obj = alloc_object();
  obj->type = MACRO;
  obj->data.macro.parameters = params;
  obj->data.macro.body = body;
  obj->data.macro.env = env;

  return obj;
}

object *make_begin(object *exp) {
  return cons(begin_symbol, exp);
}

char is_begin(object *exp) {
  return is_tagged_list(exp, begin_symbol);
}

object *begin_actions(object *exp) {
  assert( is_list(exp) );
  return cdr(exp);
}

char is_last_exp(object *exps) {
  assert( is_list(exps) );
  return is_nil(cdr(exps));
}

object *sequence_to_exp(object *seq) {
  if(is_nil(seq))
    return seq;
  else if(is_nil(cdr(seq)))
    return car(seq);
  else
    return cons(begin_symbol, seq);
}
      

object *make_if(object *pred,
                object *conseq,
                object *alt) {
  object *result = nil;
  push_stack_root(&result);
  result = cons(alt, nil);
  result = cons(conseq, result);
  result = cons(pred, result);
  result = cons(if_symbol, result);
  pop_stack_root();
  return result;
}

char is_cond(object *exp) {
  return is_tagged_list(exp, cond_symbol);
}

object *expand_clauses(object *clauses) {
  object *first;
  object *rest;

  if(is_nil(clauses)) {
    return nil;
  }
  else {
    first = car(clauses);
    rest = cdr(clauses);
    if(car(first) == else_symbol) {
      if(is_nil(rest)) {
        return sequence_to_exp(cdr(first));
      }
      else {
        error("Else clause not last.");
      }
    }
    else {
      return make_if(car(first),
                     sequence_to_exp(cdr(first)),
                     expand_clauses(rest));
    }
  }
}

object *cond_to_if(object *exp) {
  assert( is_list(exp) );
  return expand_clauses(cdr(exp));
}

char is_application(object *exp) {
  return is_cons(exp);
}

object *operator(object *exp) {
  assert( is_list(exp) );
  return car(exp);
}

object *operands(object *exp) {
  assert( is_list(exp) );
  return cdr(exp);
}

char is_no_operands(object *ops) {
  return is_nil(ops);
}

object *first_operand(object *ops) {
  assert( is_list(ops) );
  return car(ops);
}

object *rest_operands(object *ops) {
  assert( is_list(ops) );
  return cdr(ops);
}

object *eval_sequence(object *exps, object *env) {
  assert( is_list(exps) );
  object *result = nil;
  push_stack_root(&result);
  while(!is_last_exp(exps)) {
    result = eval(car(exps), env);
    exps = cdr(exps);
  }
  result = eval(car(exps), env);
  pop_stack_root();
  return result;
}

object *list_of_values(object *exps, object *env) {
  assert( is_list(exps) );
  object *result1 = nil;
  object *result2 = nil;
  if (is_no_operands(exps)) {
    return nil;
  }
  else {
    push_stack_root(&result1);
    result1 = eval(first_operand(exps), env);
    push_stack_root(&result2);
    result2 = list_of_values(rest_operands(exps), env);
    result1 = cons(result1, result2);
    pop_stack_root();
    pop_stack_root();
    return result1;
  }
}

object *copy_list(object *list) {
  assert( is_list(list) );
  object *iterator = nil;
  object *new_list = nil;

  iterator = list;
  push_stack_root(&new_list);
  new_list = nil;
  while(!is_nil(iterator)) {
    new_list = cons(car(list), new_list);
    iterator = cdr(iterator);
  }
  pop_stack_root();
  return reverse(new_list);
}

object *parse_args(object *args, object *params) {
  assert( is_list(args) );
  assert( is_list(params) );
  //object *args_copy = copy_list(args);
  object *args_copy = args;
  object *arg_iterator = args_copy;
  object *param_iterator = params;
  
  // fix the args
  if(is_nil(args_copy)) {
    return args_copy;
  }
  else if( is_eq(car(params), rest_keyword) ) {
    return cons(args_copy, nil);
  }
  else {
    while(!is_nil(cdr(param_iterator))) {
      if (is_eq(cadr(param_iterator), rest_keyword)) {
        cdr(arg_iterator) = cons(cdr(arg_iterator), nil);
        break;
      }
      arg_iterator = cdr(arg_iterator);
      param_iterator = cdr(param_iterator);
    }
    return args_copy;
  }
}

object *parse_params(object *params) {
  assert( is_list(params) );
  object *param_iterator = nil;
  object *cleaned_params = nil;
  object *reversed_cleaned_params = nil;

  // fix the params
  param_iterator = params;
  cleaned_params = nil;
  push_stack_root(&cleaned_params);
  while(!is_nil(param_iterator)) {
    if(!is_eq(car(param_iterator),rest_keyword))
      cleaned_params = cons(car(param_iterator),cleaned_params);
    param_iterator = cdr(param_iterator);
  }
  reversed_cleaned_params = reverse(cleaned_params);
  pop_stack_root();
  return reversed_cleaned_params;
}

object *apply(object *proc, object *args, object *env) {
  assert( is_list(args) );
  object *exp;
  object *parsed_args = nil;
  object *parsed_params = nil;
  object *result = nil;
  if (is_primitive_proc(proc))
    return (proc->data.primitive_proc.fn)(args, env);
  else if (is_compound_proc(proc)) {
    push_stack_root(&parsed_args);
    parsed_args = parse_args(args, proc->data.compound_proc.parameters);
    push_stack_root(&parsed_params);
    parsed_params = parse_params(proc->data.compound_proc.parameters);
    exp = proc->data.compound_proc.body;
    push_stack_root(&result);
    result = eval_sequence(exp,
                         extend_environment(
                           parsed_params,
                           parsed_args,
                           proc->data.compound_proc.env));
    pop_stack_root();
    pop_stack_root();
    pop_stack_root();
    return result;
  }
  else {
    #ifdef DEBUG
    write(proc, stdout_stream, env);
    write(args, stdout_stream, env);
    #endif
    error("Unknown procedure type");
  }
}
object *macroexpand(object *proc, object *args) {
  assert( is_list(args) );
  object *body;
  object *expanded_body = nil;
  object *parsed_params = nil;
  object *parsed_args = nil;
  
  if(is_macro(proc)) {
    push_stack_root(&parsed_args);
    parsed_args = parse_args(args, proc->data.macro.parameters);
    push_stack_root(&parsed_params);
    parsed_params = parse_params(proc->data.macro.parameters);
    body = proc->data.macro.body;
    push_stack_root(&expanded_body);
    expanded_body = eval_sequence(body,
                                  extend_environment(
                                    parsed_params,
                                    parsed_args,
                                    proc->data.macro.env));
    pop_stack_root();
    pop_stack_root();
    pop_stack_root();
    return expanded_body;
  }
  //else if(proc->data.macro.expanded) {
  //  expanded_body = body;
  //}
  else {
    error("Macro not found.");
  }
    

}

object *macroexpand_proc(object *exps, object *env) {
  assert( is_list(exps) );
  object *macro_form;
  object *proc;
  object *macro_args;
  macro_form = car(exps);
  proc = eval(car(macro_form), the_global_environment);  //will only work at the top level
  assert( is_macro(proc) );
  macro_args = cdr(macro_form);
  return macroexpand(proc, macro_args);
}

object *apply_macro(object *proc, object *args, object *env) {
  assert( is_macro(proc) );
  assert( is_list(args) );
  object *expanded_body = macroexpand(proc, args);
  return eval(expanded_body, env);
}

object *apply_proc(object *args, object *env) {
  assert( is_list(args) );
  error("Apply proc actually called.");
}

object *eval_proc(object *args, object *env) {
  assert( is_list(args) );
  error("Eval proc actually called.");
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

char is_backquoted(object *exp) {
  return is_tagged_list(exp, backquote_symbol);
}

char is_escaped(object *exp) {
  return is_tagged_list(exp, comma_symbol);
}

char is_spliced(object *exp) {
  return is_tagged_list(exp, comma_at_symbol);
}

object *eval_sequence_head(object *exp, object *env) {
  assert( is_list(exp) );
  object *head = nil;
  object *this = nil;
  object *result = nil;
  push_stack_root(&result);
  result = eval(car(exp), env);
  result = cons(result, nil);
  head = result;
  this = head;
  exp = cdr(exp);
  while(!is_nil(exp)) {
    result = eval(car(exp), env);
    result = cons(result, nil);
    this->data.cons.rest = result;
    this = result;
    exp = cdr(exp);
  }
  result = cons(head,this);
  pop_stack_root();
  return result;
}
      
object *eval_backquoted(object *exp, object *env, int backquote_depth) {
  object *thing_to_splice;
  object *head;
  object *this;
  object *cdr_eval;
  if(is_escaped(exp)) {
    //return eval(text_of_quotation(exp), env);
    return maybe_eval_backquoted(text_of_quotation(exp), env, --backquote_depth);
  }
  if(is_backquoted(exp)) {
    return eval_backquoted(text_of_quotation(exp), env, ++backquote_depth);
  }
  else if(is_cons(exp)) {
    if (is_spliced(car(exp))) {
      thing_to_splice = text_of_quotation(car(exp));
      //head = eval(thing_to_splice, env);
      head = maybe_eval_backquoted(thing_to_splice, env, --backquote_depth);
      if(is_nil(head)) {
        //return eval_backquoted(cdr(exp), env);
        return eval_backquoted(cdr(exp), env, backquote_depth);
      }
      if(!is_cons(head)) {
        printf("%d\n",thing_to_splice->type);
        error("Attempt to splice in non-cons.");
      }
      this = head;
      while(!is_nil(cdr(this)))
        this = cdr(this);
      cdr_eval = eval_backquoted(cdr(exp), env, backquote_depth);
      cdr(this) = cdr_eval;
      return head;
    }
    else {
      return cons(eval_backquoted(car(exp), env, backquote_depth),
                  eval_backquoted(cdr(exp), env, backquote_depth));
    }
  }
  else {
    return exp;
  }
}

object *maybe_eval_backquoted(object *exp, object *env, int backquote_depth) {
  if(backquote_depth > 0)
    return eval_backquoted(exp, env, backquote_depth);
  else
    return eval(exp, env);
}

object *prepare_args_for_apply(object *args) {
  if(is_nil(cdr(args)))
    return car(args);
  return cons(car(args), prepare_args_for_apply(cdr(args)));
}

object *cars_of_list(object *list) {
  return is_nil(list) ? nil : cons(caar(list), cars_of_list(cdr(list)));
}

object *cadrs_of_list(object *list) {
  return is_nil(list) ? nil : cons(cadar(list), cadrs_of_list(cdr(list)));
}

char is_let(object *exp) {
  return is_tagged_list(exp, let_symbol);
}

char is_piped(object *exp) {
  return is_tagged_list(exp, pipe_symbol);
}

object *let_to_combination(object *let_exp) {
  object *bindings;
  object *vars = nil;
  object *exps = nil;
  object *body;
  object *result = nil;
  push_stack_root(&result);
  push_stack_root(&vars);
  push_stack_root(&exps);
  bindings = cadr(let_exp);
  vars = cars_of_list(bindings);
  exps = cadrs_of_list(bindings);
  body = cddr(let_exp);

  result = make_lambda(vars, body);
  result = cons(result, exps);
  pop_stack_root();
  pop_stack_root();
  pop_stack_root();
  return result;
}

object *eval(object *exp, object *env) {
  object *proc = nil;
  object *args = nil;
  object *result = nil;
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
    else if (is_backquoted(exp)) {
      return eval_backquoted(text_of_quotation(exp), env, 1);
    }
    else if (is_piped(exp)) {
      exp = eval(text_of_quotation(exp), env);
    }
    else if (is_assignment(exp)) {
      return eval_assignment(exp, env);
    }
    else if (is_definition(exp)) {
      return eval_definition(exp,env);
    }
    else if (is_if(exp)) {
      exp = !is_nil(eval(if_predicate(exp), env)) ? if_consequent(exp) : if_alternative(exp);
    }
    else if (is_cond(exp)) {
      exp = cond_to_if(exp);
    }
    else if (is_let(exp)) {
      exp = let_to_combination(exp);
    }
    else if (is_begin(exp)) {
      return eval_sequence(begin_actions(exp), env);
    }
    else if (is_lambda(exp)) {
      return make_compound_proc(lambda_parameters(exp),
                                lambda_body(exp),
                                env);
    }
    else if (is_macro_def(exp)) {
      return make_macro(lambda_parameters(exp),
                        lambda_body(exp),
                        env);
    }
    else if (is_application(exp)) {
      push_stack_root(&proc);
      proc = eval(operator(exp), env);
      push_stack_root(&args);
      args = operands(exp);
      if(is_primitive_proc(proc) && proc->data.primitive_proc.fn == eval_proc) {
        pop_stack_root();
        pop_stack_root();
        exp = eval(car(args), env);
        env = eval(cadr(args), env);
        continue;
      }
      if(is_primitive_proc(proc) && proc->data.primitive_proc.fn == apply_proc) {
        proc = eval(car(args), env);
        args = eval(prepare_args_for_apply(cdr(args)), env);
      }
      if(is_macro(proc)) {
        push_stack_root(&result);
        result = apply_macro(proc, args, env);
        pop_stack_root();
        pop_stack_root();
        pop_stack_root();
        return result;
      }
      else {
        args = list_of_values(args, env);
        //if(is_primitive_proc(proc) &&
        //   (proc->data.primitive_proc.fn == write_proc ||
        //    proc->data.primitive_proc.fn == read_proc))
        //  args = cons(env, args);
        push_stack_root(&result);
        result = apply(proc, args, env);
        pop_stack_root();
        pop_stack_root();
        pop_stack_root();
        return result;
      }
    }
    else {
      error("Cannot eval unknown expression type.");
    }
  }
  error("Eval illegal state.");
  exit(1);
}

/*********/
/* print */
/*********/

void write_pair(object *cons, object *out_stream, object *env) {
  FILE *out;
  assert( is_list(cons) );

  if(out_stream == stdout_stream)
    out_stream = eval(stdout_symbol, env);
  out = out_stream->data.stream.fp;
  
  write(car(cons), out_stream, env);
  if((cdr(cons))->type == CONS) {
    fprintf(out," ");
    write_pair(cdr(cons), out_stream, env);
  }
  else if ( (cdr(cons))->type == NIL)
    return;
  else {
    fprintf(out," . ");
    write(cdr(cons), out_stream, env);
  }
}

void write(object *obj, object *out_stream, object *env) {
  FILE *out;
  if(out_stream == stdout_stream)
    out_stream = eval(stdout_symbol, env);
  out = out_stream->data.stream.fp;
  switch(obj->type) {
  case NIL:
    fprintf(out,"()");
    break;  
  case SYMBOL:
    fprintf(out,"%s",obj->data.symbol.value);
    break;
  case KEYWORD:
    fprintf(out,"%s", obj->data.keyword.value);
    break;
  case FIXNUM:
    fprintf(out,"%ld",obj->data.fixnum.value);
    break;
  case CHARACTER:
    fprintf(out,"#%c",obj->data.character.value);
    break;
  case STRING:
    fprintf(out,"\"%s\"",obj->data.string.value);
    break;
  case CONS:
    fprintf(out,"(");
    write_pair(obj, out_stream, env);
    fprintf(out,")");
    break;
  case PRIMITIVE_PROC:
  case COMPOUND_PROC:
    fprintf(out,"#<procedure>");
    break;
  case MACRO:
    fprintf(out,"#<macro>");
    break;
  case STREAM:
    fprintf(out,"#<stream>");
    break;
  default:
    error("Cannot write unknown type.");
  }
}

object *write_proc(object *args, object *env) {
  assert( is_list(args) );
  //object *env;
  object *obj, *out_stream;

  obj = car(args);
  if(is_nil(cdr(args)))
    //out_stream = eval(make_symbol("*stdout*"), env);
    out_stream = eval(stdout_symbol, env);
  else
    out_stream = cadr(args);

  assert( is_stream(out_stream) );

  write(obj, out_stream, env);
  fprintf(out_stream->data.stream.fp, "\n");

  return t_symbol;
}

void pad(int depth) {
  int i;
  for(i=0;i<depth;i++) {
    printf("\t");
  }
}

int print_structure(object *obj, int depth) {
  int i;
  switch(obj->type) {
    case NIL:
      printf("|/|");
      break;

    case CONS:
      printf("|.|");
      if( is_nil(cdr(obj)) ) {
          printf("|/|");
        }
      else {
        printf("|.|->\t");
        print_structure(cdr(obj),++depth);
      }
      printf("\n");
      pad(depth);
      printf("|\n");
      pad(depth);
      printf("v\n");
      pad(depth);
      print_structure(car(obj),depth);
      break;
    case FIXNUM:
      pad(depth);
      printf("%d",obj->data.fixnum.value);
      break;
    case SYMBOL:
      pad(depth);
      printf("%s",obj->data.symbol.value);
      break;
    default:
      pad(depth);
      printf("x");
      break;
    }
  printf("\n");
}



/********/
/* repl */
/********/

void read_eval_file(object* in_stream) {
  object *obj;
  while((obj = read(in_stream, the_global_environment)) != eof_object) {
    eval(obj, the_global_environment);
  }
}

void read_eval_print_file(object *in_stream, object *out_stream) {
  object *obj;
  while((obj = read(in_stream, the_global_environment)) != eof_object) {
    write(eval(obj, the_global_environment), out_stream, the_global_environment);
    printf("\n");
  }
}

void repl() {
  object *read_lisp_thing;
  object *evaled_lisp_thing;
  object *out_stream;
  printf("C-c to exit.\n");
  out_stream = eval(stdout_symbol, the_global_environment);
  while(1) {
    fprintf(stdout_stream->data.stream.fp,"=> ");
    read_lisp_thing = read(stdin_stream, the_global_environment);
    evaled_lisp_thing = eval(read_lisp_thing, the_global_environment);
    write(evaled_lisp_thing, stdout_stream, the_global_environment);
    out_stream = eval(stdout_symbol, the_global_environment);
    fprintf(out_stream->data.stream.fp,"\n");
  }
}

int main() {
  char bootstrap_code_fname[128] = "bootstrap.l";
  object *bootstrap_stream;
  printf("Iota-Bootstrap.\n");
  
  printf("Initializing core...\n");
  init();
  
  printf("Bootstrapping iota...\n");
  bootstrap_stream = make_file_stream(bootstrap_code_fname, INPUT);
  read_eval_file(bootstrap_stream);
  close_stream(bootstrap_stream);

  repl();

  return 0;
}
