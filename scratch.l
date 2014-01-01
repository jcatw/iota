
;;(begin 1 2 3)
;;(define (map f l)
;;  (define (mapper l)
;;    (if (null? l) '()
;;      (cons (f (car l)) (mapper (cdr l)))))
;;  (mapper l))

;;(define (macroexpand form)
;;  `,(macroexpand-f `,form)
