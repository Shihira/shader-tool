;; scene.scm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-model
; (mesh-gen-uv-sphere 2 20 10 #t))
 (mesh-gen-box 2 2 2))
; (vector-ref (built-in-model "teapot") 0))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define main-shader
  (built-in-shader "blinn-phong"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define propset-illum
  (make-instance propset '()
    (append (mat-cvec -2 3 4 1))
    (append (mat-cvec  1 1 1 1))
    (append (mat-cvec  0 1 5 1))))

(define propset-material
  (make-instance propset '()
    (append (make-color #xff211919))
    (append (make-color #xffb2b2b2))
    (append (make-color #xff7f3333))))

(define transfrm-mesh
  (make-instance transfrm '()
    (rotate (/ pi 6) 'zOx)
    (rotate (/ pi -6) 'yOz)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-cam (make-camera))
(transfrm-translate (camera-transformation main-cam) 0 0 5)
(render-target-set-bgcolor main-cam (color-from-value #xff000000))
(render-target-set-depth-test main-cam #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define render-rtask
  (make-instance shading-rtask '()
    (set-attributes main-model)
    (set-property "material" propset-material)
    (set-property "illum" propset-illum)
    (set-property-transfrm "transfrm" transfrm-mesh)
    (set-property-camera "camera" main-cam)
    (set-shader main-shader)
    (set-target main-cam)))

(define clear-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         (render-target-clear-buffer main-cam 'color-buffer)
         (render-target-clear-buffer main-cam 'depth-buffer)))))

(define main-rtask
  (make-instance queue-rtask '()
    (append clear-rtask)
    (append render-rtask)))

