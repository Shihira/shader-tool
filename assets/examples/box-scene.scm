(import (shrtool))

(define matrix-model (mat-zeros 4 4))
(define matrix-view
  (mat*
    (mat-rotate-4 'x (/ pi 6))
    (mat-rotate-4 'y (/ pi 6))))
(define matrix-projection
  (mat-perspective-4 (/ pi 2) (/ 4 3) 1 100))

(define func-prop
  (lambda (old-values update-info)
    (if (null? old-values)
      (list
        #t
        (list 'property-group
          (cons 'name "transfrm")
          (cons 'layout (list
                         (mat* matrix-projection
                               matrix-view
                               matrix-model)
                         matrix-model)))
        (list 'property-group
          (cons 'name "illum")
          (cons 'layout (list
                         (mat-cvec 0 3 4 1)
                         (mat-cvec 1 1 1 1)
                         (mat-cvec 0 1 5 1))))
        (list 'property-group
          (cons 'name "material")
          (cons 'layout (list
                         (color 0.1 0.1 0.13 1)
                         (color 0.1 0.1 0.13 1)
                         (color 0.1 0.1 0.13 1)))))
      (cons #f old-values))))

(define func-update
  (lambda (old-values old-update-info)
    (cons #f old-update-info)))

(define scene-main
  (list
    (list 'render-node
      (cons 'name "rn-main")
      (cons 'shader (built-in-shader "blinn-phong"))
      (cons 'attribute (built-in-model "box"))
      (cons 'properties func-prop)
      (cons 'update func-update))))

