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
(define propset-material
  (make-instance propset '()
    (append (make-color #xff211919))
    (append (make-color #xffb2b2b2))
    (append (make-color #xff7f3333))))

(define propset-illum
  (make-instance propset '()
    (append (mat-cvec -2 3 4 1))
    (append (mat-cvec  1 1 1 1))
    (append (mat-cvec  0 1 5 1))))

(define transfrm-mesh
  (make-instance transfrm '()
    (rotate (/ pi 6) 'zOx)
    (rotate (/ pi -6) 'yOz)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-cam
  (make-instance camera '()
    (set-bgcolor (color-from-value #xff332222))
    (set-depth-test #t)))
(transfrm-translate (camera-transformation main-cam) 0 0 5)

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
    (list (lambda ()
            ($ main-cam : clear-buffer 'color-buffer)
            ($ main-cam : clear-buffer 'depth-buffer)))))

(define main-rtask
  (make-instance queue-rtask '()
    (append clear-rtask)
    (append render-rtask)))

