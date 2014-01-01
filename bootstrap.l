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

(define (make-begin seq)
  (cons 'begin seq))

(define (sequence->exp seq)
  (if (null? seq) seq
      (if (null? (cdr seq)) (car seq)
	  (make-begin seq))))

(define (make-if predicate consequent alternative)
  (list 'if predicate consequent alternative))
(define (cond? exp) (tagged-list? exp 'cond))
(define (cond-clauses exp) (cdr exp))
(define (cond-predicate clause) (car clause))
(define (cond-else-clause? clause)
  (eq? (cond-predicate clause) 'else))
(define (cond-actions clause) (cdr clause))
(define (cond->if exp) (expand-clauses (cond-clauses exp)))
(define (expand-clauses clauses)
  (if (null? clauses) #f
      (let ((first (car clauses))
	    (rest (cdr clauses)))
	(if (cond-else-clause? first)
	    (if (null? rest) (sequence->exp (cond-actions first)) 'error)
	    (make-if (cond-predicate first)
		     (sequence->exp (cond-actions first))
		     (expand-clauses rest))))))
(define cond
  (macro (:rest clauses) (expand-clauses clauses)))

(define rester
  (macro (:rest clauses) `,clauses))

(define rester2
  (macro (clauses) `,clauses))

(define (rester3 :rest clauses) clauses)

"End bootstrap.l"
