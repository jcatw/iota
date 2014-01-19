"Begin bootstrap.l"

(define (error msg)
  (write msg *stdout*))

(define *stdin-std* *stdin*)
(define *stdout-std* *stdout*)

(define (cadr l)
  (car (cdr l)))

(define (caar l)
  (car (car l)))

(define (snoc l)
  (cons (cadr l) (cons (car l) nil)))

(define and (macro (:rest args)
  (if (nil? (cdr args)) (car args)
      `(cond (,(car args) (and ,@(cdr args)))
	     (else nil)))))

(define or (macro (:rest args)
  (if (nil? (cdr args)) (car args)
      `(cond (,(car args) t)
	     (else (or ,@(cdr args)))))))

(define (map f l)
  (if (null? l) '()
      (cons (f (car l)) (map f (cdr l)))))

(define (traverse combine-fn atom-fn form)
  (cond ((null? form) nil)
	((atom? form) (atom-fn form))
	(else (combine-fn (traverse combine-fn atom-fn (car form))
			  (traverse combine-fn atom-fn (cdr form))))))

(define (walk fn form)
  (traverse cons fn form))

(define (apply-many fns form)
  (cond ((null? fns) nil)
	(else (cons ((lambda (fn) (fn form)) (car fns))
		    (apply-many (cdr fns) form)))))

(define (contains? l item)
  (traverse or (lambda (x) (eq? x item)) l))
  
(define (replace from to l)
  (walk (lambda (x) (if (eq? x from) to x)) l))

(define (nest-fns fs x)
  (cond ((null? fs) '())
        ((atom? fs) (list fs x))
        ((last-exp? fs) (list (car fs) x))
        (else (cons (car fs)
                    (cons (nest-fns (cdr fs) x) '())))))

(define (replace-many replacements l)
  (cond ((null? l) nil)
	((atom? l) (map (lambda (replacement)
			  (if (eq? l (car replacement)) (cadr replacement) l))
			replacements))
	(else (cons (replace-many replacements (car l))
		    (replace-many replacements (cdr l))))))

(define (inc x) (+ x 1))

(define *gensym-counter* 0)
(define (gensym)
  (set! *gensym-counter* (inc *gensym-counter*))
  (string->symbol (strcat (strcat "-gensym-" (number->string *gensym-counter*)) "-")))

(define with-gensyms
  (macro (names :rest body)
    `(let ,(map (lambda (n) `(,n (gensym))) names)
       ,@body)))

(define defmacro
  (macro (macro-name macro-args :rest macro-body)
    `(define ,macro-name
       (macro ,macro-args
	 ,@macro-body))))

(defmacro defn
  (fn-name fn-args :rest fn-body)
  `(define (,fn-name ,@fn-args)
     ,@fn-body))

(define (make-table)
  (define (assoc key recrds)
    (cond ((nil? recrds) nil)
          ((eq? key (caar recrds)) (car recrds))
          (else (assoc key (cdr recrds)))))
  (let ((table (list '*table*)))
    (define (lookup key1 key2)
      (let ((subtable (assoc key1 (cdr table))))
        (if subtable
            (let ((record (assoc key2 (cdr subtable))))
              (if record
                  (cdr record)
                nil))
          nil)))
    (define (insert! key1 key2 value)
      (let ((subtable (assoc key1 (cdr table))))
        (if subtable
            (let ((record (assoc key2 (cdr subtable))))
              (if record
                  (set-cdr! record value)
                  (set-cdr! subtable
                            (cons (cons key2 value)
                                  (cdr subtable)))))
            (set-cdr! table
                      (cons (list key1 (cons key2 value))
                            (cdr table)))))
      t)
    (define (dispatch m)
      (cond ((eq? m :lookup) lookup)
            ((eq? m :insert) insert!)
            ((eq? m :inspect) (lambda () table))
            ((eq? m :assoc) (lambda (k) (assoc k (cdr table))))
            ((eq? m :assoc2) (lambda (k1 k2) (assoc k2 (assoc k1 (cdr table)))))
            (else (error "Unknown table op"))))
    dispatch))

(define (last-exp? seq)
  (null? (cdr seq)))

(define (do f l)
  (cond ((null? l) nil)
        ((atom? l) (f l))
        ((last-exp? l) (f (car l)))
        (else (begin (f (car l)) (do f (cdr l))))))

(define (consify l)
  (if (last-exp? l) l
      (cons (car l)
            (cons (consify (cdr l)) '()))))

(define compose
  (macro (:rest fs)
    `(lambda (x) ,(nest-fns fs 'x))))

(define writing-to
  (macro (stream :rest body)
  `(let ((*stdout* ,stream))
     ,@body)))

(define writing-to-file
  (with-gensyms (stream)
  (macro (file-name :rest body)
  `(let ((,stream (make-file-stream ,file-name :output)))
     (let ((*stdout* ,stream))
       ,@body)
     (close-stream ,stream)))))

(define reading-from
  (macro (stream :rest body)
  `(let ((*stdin* ,stream))
     ,@body)))

(define reading-from-file
  (with-gensyms (stream)
  (macro (file-name :rest body)
   `(let ((,stream (make-file-stream ,file-name :input)))
      (let ((*stdin* ,stream))
	,@body)
      (close-stream ,stream)))))
  
"End bootstrap.l"
