(import (shrtool))

(define meshes
  (list (mesh-gen-box 2 2 2)
        (mesh-gen-plane 10 10 1 1)
        (mesh-gen-box 3 0.2 10)
        (mesh-gen-box 10 0.2 3)
        (mesh-gen-uv-sphere 0.5 20 10 #t)
        (mesh-gen-uv-sphere 0.8 10 5 #f)))

(define mesh-transfrms
  (list
    (make-instance transfrm '()
      (rotate (/ pi 6) 'zOx)
      (translate 0 0 0))
    (make-instance transfrm '()
      (translate 0 -1 0))
    (make-instance transfrm '()
      (rotate (/ pi 2) 'xOy)
      (translate 5 0.50 0))
    (make-instance transfrm '()
      (rotate (/ pi -2) 'yOz)
      (translate 0 0.50 -5))
    (make-instance transfrm '()
      (translate -0.5 1.5 0))
    (make-instance transfrm '()
      (translate 2 -0.05 1))))

(define shadow-map-tex
  (make-instance texture2d '(1024 1024)
    (reserve 'depth-f32)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define shadow-map-shader
  (shader-from-config (load "shadow-map.scm")))

(define light-transfrm
  (make-instance transfrm '()
    (translate 0 0 10)
    (rotate (/ pi 6) 'yOz)
    (rotate (/ pi -1.7) 'zOx)))

(define light-cam
  (make-instance camera '()
    (attach-texture 'depth-buffer shadow-map-tex)
    (set-visible-angle (/ pi 1.8))
    (set-depth-test #t)
    (set-transformation light-transfrm)))

(define shadow-map-rtasks
  (map (lambda (msh tf)
         (make-instance shading-rtask '()
           (set-attributes msh)
           (set-property-transfrm "transfrm" tf)
           (set-property-camera "camera" light-cam)
           (set-shader shadow-map-shader)
           (set-target light-cam)))
    meshes mesh-transfrms))

(define shadow-map-display-rtask
  (rtask-def-display-texture shadow-map-tex #:channel 'r))
($ (cadr (assq 'target (cdr shadow-map-display-rtask))) : set-viewport
  (rect-from-size 200 200))

(define shadow-map-clear-rtask
  (rtask-def-clear light-cam '(depth-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-shader
  (shader-from-config (load "blinn-phong-with-shadow.scm")))

(define cam-transfrm
  (make-instance transfrm '()
    (translate 0 0 20)
    (rotate (/ pi 8) 'yOz)
    (rotate (/ pi 12) 'zOx)))

(define main-cam
  (make-instance camera '()
    (set-depth-test #t)
    (set-visible-angle (/ pi 5))
    (set-transformation cam-transfrm)
    (set-bgcolor #xff222222)))

(define propset-illum
  (make-instance propset '()
    (append-float 0.003)
    (append 2)
    (append (make-color #xffffffff))))

(define propset-material
  (make-instance propset '()
    (append (make-color #xff211919))
    (append (make-color #xffb2b2b2))
    (append (make-color #xff7f3333))))

(define screen-rtasks
  (map (lambda (msh tf)
         (make-instance shading-rtask '()
           (set-attributes msh)
           (set-property-transfrm "transfrm" tf)
           (set-property-camera "camera" main-cam)
           (set-property-camera "lightCamera" light-cam)
           (set-property "illum" propset-illum)
           (set-property "material" propset-material)
           (set-texture "shadowMap" shadow-map-tex)
           (set-shader main-shader)
           (set-target main-cam)))
    meshes mesh-transfrms))

(define main-clear-rtask
  (rtask-def-clear main-cam '(depth-buffer color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-rtask
  (make-instance queue-rtask '()
    (append (car shadow-map-clear-rtask))
    (append (car main-clear-rtask))
    (append* shadow-map-rtasks)
    (append* screen-rtasks)
    (append (car shadow-map-display-rtask))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; utility functions

(define rotate-light
  (lambda* (#:key (angle (/ pi 120)))
    ($- ($ light-cam : transformation) : rotate angle 'zOx)))

