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

(define transfrm-mesh
  (make-transfrm))
(transfrm-rotate transfrm-mesh (/ pi 6) 'zOx)
(transfrm-rotate transfrm-mesh (/ pi -6) 'yOz)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-cam
  (make-camera))
(transfrm-translate (camera-transformation main-cam) 0 0 5)
(render-target-set-bgcolor main-cam (color-from-value #xff333333))
(render-target-set-depth-test main-cam #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-rtask
  (make-shading-rtask))
(shading-rtask-set-attributes main-rtask main-model)
(shading-rtask-set-property main-rtask "material" propset-material)
(shading-rtask-set-property main-rtask "illum" propset-illum)
(shading-rtask-set-property-transfrm main-rtask "transfrm" transfrm-mesh)
(shading-rtask-set-property-camera main-rtask "camera" main-cam)
(shading-rtask-set-shader main-rtask main-shader)
(shading-rtask-set-target main-rtask main-cam)

