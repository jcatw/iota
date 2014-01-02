"Begin bootstrap.l"

(define (cadr l)
  (car (cdr l)))

(define (map f l)
  (if (null? l) '()
      (cons (f (car l)) (map f (cdr l)))))

(define (make-begin seq)
  (cons 'begin seq))

(define (sequence->exp seq)
  (if (null? seq) seq
      (if (null? (cdr seq)) (car seq)
	  (make-begin seq))))

(define let
  (macro (assignments :rest body)
    `((lambda ,(map car assignments)
	,@body)
      ,@(map cadr assignments))))

(define (make-if predicate consequent alternative)
  (list 'if predicate consequent alternative))

(define cond
  (macro (:rest clauses)
    (let ((clause (car clauses))
	  (other-clauses (cdr clauses)))
      `(if ,@clause ,(cond other-clauses)))))

"End bootstrap.l"
