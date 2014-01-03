"Begin bootstrap.l"

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

"End bootstrap.l"
