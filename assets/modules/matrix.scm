;; Copyright(C) Shihira Fung <fengzhiping@hotmail.com>

(import (rnrs (6)))
(import (srfi srfi-1))
(import (ice-9 format))

(define pi 3.14159265358979)

(define mat-zeros
  (lambda (m n)
    (make-typed-array 'f64 0 m n)))

(define mat?
  (lambda (A)
    (and
      (eq? (array-type A) 'f64)
      (eq? (array-rank A) 2))))

(define mat-size
  (lambda (A)
    (assert (mat? A))
    (array-dimensions A)))

(define mat-zeros-like
  (lambda (A)
    (apply mat-zeros (mat-size A))))

(define mat-col-count
  (lambda (A)
    (cadr (mat-size A))))

(define mat-row-count
  (lambda (A)
    (car (mat-size A))))

(define mat-cvec?
  (lambda (v)
    (and
      (mat? v)
      (= (mat-col-count v) 1))))

(define mat-rvec?
  (lambda (v)
    (and
      (mat? v)
      (= (mat-row-count v) 1))))

(define mat-diagonal
  (lambda (vec)
    (assert (mat-rvec? vec))
    (let*
      ((dim (mat-col-count vec))
       (M (mat-zeros dim dim)))
      (array-index-map! vec
        (lambda (r c)
          (let ((val (array-ref vec r c)))
            (array-set! M val c c)
            val)))
      M)))

(define mat-eye
  (lambda (dim)
    (mat-diagonal (make-typed-array 'f64 1 1 dim))))

(define mat-translate-4
  (lambda (x y z)
    (let ((M (mat-eye 4)))
      (array-set! M x 0 3)
      (array-set! M y 1 3)
      (array-set! M z 2 3)
      M)))

(define mat-scale-4
  (lambda (x y z)
    (mat-diagonal (list->rvec (list x y z 1)))))

(define mat-rotate-4
  (lambda (axis angle)
    (let ((c (cos angle))
          (s (sin angle))
          (n (- (sin angle))))
      (cond
        ((eq? axis 'x)
         (make-mat 1 0 0 0 ':
                   0 c s 0 ':
                   0 n c 0 ':
                   0 0 0 1 ':))
        ((eq? axis 'y)
         (make-mat c 0 n 0 ':
                   0 1 0 0 ':
                   s 0 c 0 ':
                   0 0 0 1 ':))
        ((eq? axis 'z)
         (make-mat c s 0 0 ':
                   n c 0 0 ':
                   0 0 1 0 ':
                   0 0 0 1 ':))
        (else (error "mat-rotate-4" "not supported axis"))))))

(define mat+
  (lambda (A B)
    (let ((M (mat-zeros-like A)))
      (array-map! M (lambda (a b) (+ a b)) A B)
      M)))

(define mat-
  (lambda (A B)
    (let ((M (mat-zeros-like A)))
      (array-map! M (lambda (a b) (- a b)) A B)
      M)))

(define mat-t
  (lambda (A)
    (let ((M (apply mat-zeros (reverse (mat-size A)))))
      (array-index-map! M (lambda (r c) (array-ref A c r)))
      M)))

(define mat-eq? array-equal?)

(define list->mat
  (lambda (l)
    (list->typed-array 'f64 '(0 0) l)))

(define make-mat
  (lambda (. l)
    (let* ((res (fold (lambda (e l)
                 (let ((working-list (car l))
                       (done-list (cadr l)))
                   (if (eq? e ':)
                     (list '() (cons (reverse working-list) done-list))
                     (list (cons e working-list) done-list))))
                 '(()()) l))
           (final (if (null? (car res))
                    (reverse (cadr res))
                    (reverse (cons (reverse (car res)) (cadr res))))))
      (list->mat final))))

(define list->rvec
  (lambda (l)
    (list->mat (list l))))

(define list->cvec
  (lambda (l)
    (mat-t (list->rvec l))))

(define mat-cvec
  (lambda (. l)
    (list->cvec l)))

(define mat-rvec
  (lambda (. l)
    (list->rvec l)))

(define mat-index-fold
  (lambda (proc init A)
    (letrec* ((size (mat-size A))
              (r (car size)) (c (cadr size))
              (iter (lambda (val i j)
                      (if (>= (+ j 1) c)
                        (if (>= (+ i 1) r) (proc i j val)
                          (iter (proc i j val) (+ i 1) 0))
                        (iter (proc i j val) i (+ j 1))))))
      (iter init 0 0))))

(define mat-index-fold4
  (lambda (proc init A)
    (mat-index-fold (lambda (r c val)
                      (proc r c val (mat-ref A r c))) init A)))

(define mat-ref array-ref)

(define mat-slice
  (lambda (A rngr rngc)
    (let* ((reg-range (lambda (rng) rng))
           (reg-rngr (reg-range rngr))
           (reg-rngc (reg-range rngc))
           (r (- (cadr reg-rngr) (car reg-rngr)))
           (c (- (cadr reg-rngc) (car reg-rngc)))
           (M (mat-zeros r c)))
     (array-index-map! M
       (lambda (r c)
         (mat-ref A (+ r (car reg-rngr)) (+ c (car reg-rngc)))))
     M)))

(define mat*
  (lambda (A B)
    (if (number? B)
      (let ((M (mat-zeros-like A)))
        (array-map! M (lambda (e) (* e B)) A)
        M)
      (begin
        (assert (= (mat-col-count A) (mat-row-count B)))
        (letrec* ((M (mat-zeros (mat-row-count A) (mat-col-count B)))
                  (n (mat-col-count A))
                  (iter (lambda (ra cb i val)
                          (if (< i n)
                            (iter ra cb (+ 1 i)
                              (+ val (* (mat-ref A ra i) (mat-ref B i cb))))
                            val))))
            (array-index-map! M
              (lambda (r c) (iter r c 0 0)))
            M)))))

(define mat/
  (lambda (A a)
    (assert (number? a))
    (mat* A (/ 1 a))))

(define mat-pretty
  (lambda (A . opt-port)
    (let ((max-len (mat-index-fold4
                     (lambda (r c fval e)
                       (max (string-length (number->string e)) fval))
                     0 A))
          (port (if (null? opt-port) #t opt-port)))
      (mat-index-fold4
        (lambda (r c fval e)
          (if (= c 0)
            (if (> r 0) (format port ";\n  ") (format port "[ ")) #f)
          (format port "~vf" max-len e)
          (if (< c (- (mat-col-count A) 1))
            (format port " ")))
        0 A)
      (format port " ]"))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define mat-test-cases
  (lambda ()
    (let ((Mat1 #2f64((1 2 3)
                      (4 5 4)
                      (3 2 1)))
          (Mat2 #2f64((1 2.896 3)
                      (4.567 5 4)
                      (3 2 1.234))))
      (assert (mat-eq? (mat-diagonal (mat-rvec 5 6 7))
                #2f64((5 0 0)
                      (0 6 0)
                      (0 0 7))))
      (assert (= (mat-index-fold4
                   (lambda (a b val ref) (+ val ref))
                   0 Mat1) 25))
      (assert (mat-eq? (mat-slice Mat1 '(1 3) '(0 2))
                #2f64((4 5) (3 2))))
      (begin (mat-pretty Mat2) (newline))
      (assert (mat-eq? (make-mat 1 2 ': 2 3) #2f64((1 2)(2 3))))
      (assert (mat-eq? (make-mat 1 2 ': 2 3 ':) #2f64((1 2)(2 3))))
      (assert (mat-eq? (mat-t (mat-rvec 5 8 9)) (mat-cvec 5 8 9)))
      (assert (mat-eq? (mat-zeros 2 3) #2f64((0 0 0)(0 0 0)))))))
