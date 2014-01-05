"Begin bootstrap.l"

(define defmacro
  (macro (macro-name macro-args :rest macro-body)
    `(define ,macro-name
       (macro ,macro-args
	 ,@macro-body))))

(defmacro defn
  (fn-name fn-args :rest fn-body)
  `(define (,fn-name ,@fn-args)
     ,@fn-body))

(defmacro eval
  (form)
  (cadr form))
  
(define (cadr l)
  (car (cdr l)))

(define (map f l)
  (if (null? l) '()
      (cons (f (car l)) (map f (cdr l)))))

(define let
  (macro (assignments :rest body)
    `((lambda ,(map car assignments)
	,@body)
      ,@(map cadr assignments))))

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

"End bootstrap.l"
