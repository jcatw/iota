
;;(begin 1 2 3)
;;(define (map f l)
;;  (define (mapper l)
;;    (if (null? l) '()
;;      (cons (f (car l)) (mapper (cdr l)))))
;;  (mapper l))

;;(define (macroexpand form)
;;  `,(macroexpand-f `,form)

(define let
  (macro (assignments :rest body)
    `((lambda ,(map car assignments)
	,@body)
      ,@(map cadr assignments))))

(define cond
  (macro (:rest clauses) (expand-clauses clauses)))

(define rester
  (macro (:rest clauses) `,clauses))

(define rester2
  (macro (clauses) `,clauses))

(define (rester3 :rest clauses) clauses)

