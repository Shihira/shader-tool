;; scene.scm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-model
; (mesh-gen-uv-sphere 2 20 10 #t))
 (mesh-gen-box 2 2 2))
; (vector-ref (built-in-model "teapot") 0))
(display main-model)
(newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-shader
  (built-in-shader "blinn-phong"))
(display main-shader)
(newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define propset-illum
  (make-propset))
(propset-resize propset-illum 3)
(propset-set propset-illum 0 (mat-cvec -2 3 4 1))
(propset-set propset-illum 1 (mat-cvec 1 1 1 1))
(propset-set propset-illum 2 (mat-cvec 0 1 5 1))
(display (propset-definition propset-illum "prop_illum"))
(newline)

(define propset-material
  (make-propset))
(propset-resize propset-material 3)
(propset-set propset-material 0 (mat-cvec 0.1 0.1 0.13 1))
(propset-set propset-material 1 (mat-cvec 0.7 0.7 0.7 1))
(propset-set propset-material 2 (mat-cvec 0.2 0.2 0.5 1))
(display (propset-definition propset-material "prop_illum"))
(newline)

(define matrix-model (mat* (mat-rotate-4 'x (/ pi -6))
                      (mat-rotate-4 'y (/ pi 6))))
(define matrix-view (mat-translate-4 0 0 -5))
(define matrix-projection (mat-perspective-4 (/ pi 2) (/ 4 3) 1 100))

(mat-pretty matrix-model)
(newline)
(mat-pretty matrix-view)
(newline)
(mat-pretty matrix-projection)
(newline)

(define propset-transfrm
  (make-propset))
(propset-resize propset-transfrm 2)
(propset-set propset-transfrm 0 (mat* matrix-projection
                                   matrix-view
                                   matrix-model))
(propset-set propset-transfrm 1 matrix-model)
(display (propset-definition propset-transfrm "prop_illum"))
(newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-target
    (render-target-screen))
(render-target-initial-color main-target 0.2 0.2 0.2 1)
(render-target-enable-depth-test main-target #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-rtask
  (make-shading-rtask))
(shading-rtask-set-attributes main-rtask main-model)
(shading-rtask-set-property main-rtask "material" propset-material)
(shading-rtask-set-property main-rtask "illum" propset-illum)
(shading-rtask-set-property main-rtask "transfrm" propset-transfrm)
(shading-rtask-set-shader main-rtask main-shader)
(shading-rtask-set-target main-rtask (render-target-screen))

