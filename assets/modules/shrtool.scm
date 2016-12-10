(define-module (shrtool)
  #:export (
      ;; shader-def
      attr-mesh
      prop-transfrm
      
      ;; matrix
      pi
      mat-zeros
      mat?
      mat-size
      mat-zeros-like
      mat-col-count
      mat-row-count
      mat-cvec?
      mat-rvec?
      mat-diagonal
      mat-eye
      mat-translate-4
      mat-scale-4
      mat-rotate-4
      mat+
      mat-
      mat-t
      mat-eq?
      list->mat
      make-mat
      list->rvec
      list->cvec
      mat-cvec
      mat-rvec
      mat-index-fold
      mat-index-fold4
      mat-ref
      mat-slice
      mat*
      mat-pretty
  ))

(include "shader-def.scm")
(include "matrix.scm")
