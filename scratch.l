
;;(begin 1 2 3)
;;(define (map f l)
;;  (define (mapper l)
;;    (if (null? l) '()
;;      (cons (f (car l)) (mapper (cdr l)))))
;;  (mapper l))

;;(define (macroexpand form)
;;  `,(macroexpand-f `,form)

(define cond
  (macro (:rest clauses) (expand-clauses clauses)))

(define rester
  (macro (:rest clauses) `,clauses))

(define rester2
  (macro (clauses) `,clauses))

(define (rester3 :rest clauses) clauses)

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
(define (expand-clauses clauses)
  (if (null? clauses)
      'false                          
      (let ((first (car clauses))
            (rest (cdr clauses)))
        (if (cond-else-clause? first)
            (if (null? rest)
                (sequence->exp (cond-actions first))
                (error "ELSE clause isn't last -- COND->IF"
                       clauses))
            (make-if (cond-predicate first)
                     (sequence->exp (cond-actions first))
                     (expand-clauses rest))))))
(define (cond? exp) (tagged-list? exp 'cond))
(define (cond-clauses exp) (cdr exp))
(define (cond-else-clause? clause)
  (eq? (cond-predicate clause) 'else))
(define (cond-predicate clause) (car clause))
(define (cond-actions clause) (cdr clause))
(define (cond->if exp)
  (expand-clauses (cond-clauses exp)))
(define expand-clauses
  (macro (clauses)
    (if (null? clauses)
	#f                          
	(let ((first (car clauses))
	      (rest (cdr clauses)))
	  (if (cond-else-clause? first)
	      (if (null? rest)
		  (sequence->exp (cond-actions first))
		  (error "ELSE clause isn't last -- COND->IF"
			 clauses))
	      (make-if (cond-predicate first)
		       (sequence->exp (cond-actions first))
		       (expand-clauses rest)))))))
(define cond
  (macro (:rest clauses)
    (expand-clauses clauses)))
