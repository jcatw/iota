"Begin bootstrap.l"

(define (error msg)
  (write msg *stdout*))

(define defmacro
  (macro (macro-name macro-args :rest macro-body)
    `(define ,macro-name
       (macro ,macro-args
	 ,@macro-body))))

(defmacro defn
  (fn-name fn-args :rest fn-body)
  `(define (,fn-name ,@fn-args)
     ,@fn-body))

(define (inc x) (+ x 1))

(define *gensym-counter* 0)
(define (gensym)
  (set! *gensym-counter* (inc *gensym-counter*))
  (string->symbol (strcat "gensym-" (number->string *gensym-counter*))))

(define *stdin-std* *stdin*)
(define *stdout-std* *stdout*)
  
(defn cadr (l)
  (car (cdr l)))

(defn caar (l)
  (car (car l)))

(define (map f l)
  (if (null? l) '()
      (cons (f (car l)) (map f (cdr l)))))

(defmacro and (:rest args)
  (if (nil? (cdr args)) (car args)
      `(cond (,(car args) (and ,@(cdr args)))
	     (else nil))))

(defmacro or (:rest args)
  (if (nil? (cdr args)) (car args)
      `(cond (,(car args) t)
	     (else (or ,@(cdr args))))))

(define (thing)
  (let ((y '(1 2 3)))
    (define (z a b)
      (cons a (cons b y)))
    (define (r a)
      (set-cdr! y a))
    (define (dispatch m)
      (cond ((eq? m :z) z)
	    ((eq? m :r) r)
	    ((eq? m :y) y)))
    dispatch))
  
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

(define (nest-fns fs x)
  (cond ((null? fs) '())
	((atom? fs) (list fs x))
	((last-exp? fs) (list (car fs) x))
	(else (cons (car fs)
		    (cons (nest-fns (cdr fs) x) '())))))

(define compose
  (macro (:rest fs)
    `(lambda (x) ,(nest-fns fs 'x))))

(defmacro writing-to
  (stream :rest body)
  `(let ((*stdout* ,stream))
     ,@body))

(defmacro writing-to-file
  (file-name :rest body)
  `(let ((stream (make-file-stream ,file-name :output)))
     (let ((*stdout* stream))
       ,@body)
     (close-stream stream)))

(defmacro reading-from
  (stream :rest body)
  `(let ((*stdin* ,stream))
     ,@body))

(defmacro reading-from-file
  (file-name :rest body)
  `(let ((stream (make-file-stream ,file-name :input)))
     (let ((*stdin* stream))
       ,@body)
     (close-stream stream)))

"End bootstrap.l"
