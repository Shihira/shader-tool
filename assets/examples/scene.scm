;; scene.scm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-model
  (mesh-gen-uv-sphere 2 20 10 #t))
(display main-model)
(newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-shader
  (shader-from-config (load "../shaders/blinn-phong.scm")))
(display main-shader)
(newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define prop-illum
  (make-propset))
(propset-resize prop-illum 3)
(propset-set prop-illum 0 (mat-cvec -2 3 4 1))
(propset-set prop-illum 1 (mat-cvec 1 1 1 1))
(propset-set prop-illum 2 (mat-cvec 0 1 5 1))
(display (propset-definition prop-illum "prop_illum"))
(newline)

(define prop-material
  (make-propset))
(propset-resize prop-material 3)
(propset-set prop-material 0 (mat-cvec 0.1 0.1 0.13 1))
(propset-set prop-material 1 (mat-cvec 0.7 0.7 0.7 1))
(propset-set prop-material 2 (mat-cvec 0.2 0.2 0.5 1))
(display (propset-definition prop-material "prop_illum"))
(newline)

(define matrix-model (mat-eye 4))
(define matrix-view (mat-translate-4 0 0 -5))
(define matrix-projection (mat-perspective-4 (/ pi 2) (/ 4 3) 1 100))

(mat-pretty matrix-model)
(newline)
(mat-pretty matrix-view)
(newline)
(mat-pretty matrix-projection)
(newline)

(define prop-transfrm
  (make-propset))
(propset-resize prop-transfrm 2)
(propset-set prop-transfrm 0 (mat* matrix-projection
                                   matrix-view
                                   matrix-model))
(propset-set prop-transfrm 1 matrix-model)
(display (propset-definition prop-transfrm "prop_illum"))
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
(shading-rtask-set-property main-rtask "material" prop-material)
(shading-rtask-set-property main-rtask "illum" prop-illum)
(shading-rtask-set-property main-rtask "transfrm" prop-transfrm)
(shading-rtask-set-shader main-rtask main-shader)
(shading-rtask-set-target main-rtask (render-target-screen))

