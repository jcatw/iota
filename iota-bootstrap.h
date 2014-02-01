#ifndef BOOTSTRAP_IOTA_H
#define BOOTSTRAP_IOTA_H

typedef enum {NIL, SYMBOL, KEYWORD,
              FIXNUM, CHARACTER, STRING,
              CONS, MACRO, PRIMITIVE_PROC,
              COMPOUND_PROC, STREAM} object_type;

typedef enum {OUTPUT, INPUT} directiontype;

typedef struct object object;

// fundamental things, symbols, streams, etc
object *nil;
object *t_symbol;
object *symbol_table;
object *keyword_table;
object *quote_symbol;
object *backquote_symbol;
object *comma_symbol;
object *pipe_symbol;
object *comma_at_symbol;
object *define_symbol;
object *set_symbol;
object *if_symbol;
object *cond_symbol;
object *else_symbol;
object *lambda_symbol;
object *let_symbol;
object *begin_symbol;
object *macro_symbol;
object *rest_keyword;
object *eof_object;
object *stdin_stream;
object *stdout_stream;
object *stdin_symbol;
object *stdout_symbol;
object *output_keyword;
object *input_keyword;
object *the_empty_environment;
object *the_global_environment;

// constructors
object *alloc_object();
object *make_symbol(char *value);
object *make_keyword(char *value);
object *make_fixnum(long value);
object *make_character(char value);
object *make_string(char *value);
object *make_file_stream(char* stream_name, directiontype direction);
object *make_primitive_proc(object *(*fn)(struct object *args, struct object *env));
object *make_macro(object *params,
                   object *body,
                   object *env);
object *make_compound_proc(object *params,
                           object *body,
                           object *env);


// list manipulation internals
object *cons(object *first, object *rest);
#define car(X) ((X)->data.cons.first)
#define cdr(X) ((X)->data.cons.rest)
#define caar(X) (car(car(X)))
#define caaar(X) (car(car(car(X))))
#define cadar(X) (car(cdr(car(X))))
#define cddr(X) (cdr(cdr(X)))
#define cdddr(X) (cdr(cdr(cdr(X))))
#define cadr(X) (car(cdr(X)))
#define cdar(X) (cdr(car(X)))
#define cdadr(X) (cdr(car(cdr(X))))
#define caddr(X) (car(cdr(cdr(X))))
#define cadddr(X) (car(cdr(cdr(cdr(X)))))
#define caadr(X) (car(car(cdr(X))))
long len(object *obj);
object *list_of_values(object *exps, object *env);
object *copy_list(object *list);
object *macroexpand(object *proc, object *args);
object *cars_of_list(object *list);
object *cadrs_of_list(object *list);


//predicates
char is_eq(object *obj1, object *obj2);
char is_nil(object *obj);
char is_symbol(object *obj);
char is_keyword(object *obj);
char is_fixnum(object *obj);
char is_character(object *obj);
char is_string(object *obj);
char is_cons(object *obj);
char is_list(object *obj);
char is_atom(object *obj);
char is_stream(object *obj);
char is_output_stream(object *obj);
char is_input_stream(object *obj);
char is_primitive_proc(object *obj);
char is_tagged_list(object *expression, object *tag);
char is_compound_proc(object *obj);
char is_backquoted(object *exp);
char is_escaped(object *exp);
char is_spliced(object *exp);
char is_let(object *exp);
char is_piped(object *exp);
char is_self_evaluating(object *obj);
char is_variable(object *exp);
char is_quoted(object *expression);
char is_assignment(object *exp);
char is_definition(object *exp);
char is_if(object *exp);
char is_lambda(object *exp);
char is_macro (object *obj);
char is_macro_def(object *exp);
char is_begin(object *exp);
char is_last_exp(object *exps);
char is_cond(object *exp);
char is_application(object *exp);
char is_no_operands(object *ops);

//environment
object *make_frame(object *variables, object *values);
object *enclosing_environment(object *env);
object *first_frame(object *env);
object *frame_variables(object *frame);
object *frame_values(object *frame);
void add_binding_to_frame(object *var,
                          object *val,
                          object *frame);
object *extend_environment(object *vars,
                           object *vals,
                           object *base_env);
object *lookup_variable_value(object *var, object *env);
void set_variable_value(object *var,
                        object *val,
                        object *env);
void define_variable(object *var,
                     object *val,
                     object *env);
object *setup_environment();
object *assignment_variable(object *exp);
object *assignment_value(object *exp);
object *definition_variable(object *exp);
object *definition_value(object *exp);

//applications
object *make_lambda(object *params, object *body);
object *lambda_parameters(object *exp);
object *lambda_body(object *exp);
object *operator(object *exp);
object *operands(object *exp);
object *first_operand(object *ops);
object *rest_operands(object *ops);
object *parse_args(object *args, object *params);
object *parse_params(object *params);
object *prepare_args_for_apply(object *args);
object *apply(object *proc, object *args, object *env);
object *apply_macro(object *proc, object *args, object *env);

//compound exps
object *text_of_quotation(object *expression);
object *if_predicate(object *exp);
object *if_consequent(object *exp);
object *if_alternative(object *exp);
object *expand_clauses(object *clauses);
object *cond_to_if(object *exp);
object *make_begin(object *exp);
object *begin_actions(object *exp);
object *sequence_to_exp(object *seq);
object *make_if(object *pred,
                object *conseq,
                object *alt);
object *let_to_combination(object *let_exp);

//read
char is_delimiter(int c);
char is_initial(int c);
int peek(FILE *in);
void eat_whitespace(FILE *in);
object *read(object *in_stream, object *env);
object *read_pair(object *in_stream, object *env);

//eval
object *eval(object *exp, object *env);
object *eval_assignment(object *exp, object *env);
object *eval_definition(object *exp, object *env);
object *eval_sequence(object *exps, object *env);
object *eval_sequence_head(object *exp, object *env);
object *maybe_eval_backquoted(object *exp, object *env, int backquote_depth);
object *eval_backquoted(object *exp, object *env, int backquote_depth);

//write
void write_pair(object *cons, object *out_stream, object *env);
void write(object *obj, object *out_stream, object *env);

//misc
void error(char *msg);
void close_stream(object *stream);

//lisp-side procs
object *error_proc(object *args, object *env);
object *is_null_proc(object *args, object *env);
object *is_list_proc(object *args, object *env);
object *is_atom_proc(object *args, object *env);
object *is_symbol_proc(object *args, object *env);
object *is_keyword_proc(object *args, object *env);
object *is_integer_proc(object *args, object *env);
object *is_char_proc(object *args, object *env);
object *is_string_proc(object *args, object *env);
object *is_cons_proc(object *args, object *env);
object *is_procedure_proc(object *args, object *env);
object *is_tagged_list_proc(object *args, object *env);
object *char_to_integer_proc(object *args, object *env);
object *integer_to_char_proc(object *args, object *env);
object *number_to_string_proc(object *args, object *env);
object *string_to_number_proc(object *args, object *env);
object *concat_proc(object *args, object *env);
object *symbol_to_string_proc(object *args, object *env);
object *string_to_symbol_proc(object *args, object *env);
object *add_proc(object *args, object *env);
object *subtract_proc(object *args, object *env);
object *multiply_proc(object *args, object *env);
object *divide_proc(object *args, object *env);
object *is_equal_proc(object *args, object *env);
object *is_less_than_proc(object *args, object *env);
object *is_greater_than_proc(object *args, object *env);
object *cons_proc(object *args, object *env);
object *car_proc(object *args, object *env);
object *car_proc(object *args, object *env);
object *cdr_proc(object *args, object *env);
object *set_car_proc(object *args, object *env);
object *set_cdr_proc(object *args, object *env);
object *list_proc(object *args, object *env);
object *len_proc(object *args, object *env);
object *is_eq_proc(object *args, object *env);
object *reverse(object *head);
object *reverse_proc(object *args, object *env);
object *make_file_stream_proc(object *args, object *env);
object *close_stream_proc(object *args, object *env);
object *global_environment_proc(object *args, object *env);
object *macroexpand_proc(object *exps, object *env);
object *apply_proc(object *args, object *env);
object *eval_proc(object *args, object *env);
object *read_proc(object *args, object *env);
object *write_proc(object *args, object *env);
object *read_proc(object *args, object *env);

//bootstrap
#define add_procedure(scheme_name, c_name)      \
  define_variable(make_symbol(scheme_name),     \
                  make_primitive_proc(c_name),  \
                  the_global_environment);
void init();
void read_eval_file(object* in_stream);
void read_eval_print_file(object *in_stream, object *out_stream);
void repl();

#endif /* BOOTSTRAP_IOTA_H */
