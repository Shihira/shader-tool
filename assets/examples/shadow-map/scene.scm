(import (shrtool))

(define meshes
  (list (mesh-gen-box 2 2 2)
        (mesh-gen-plane 10 10 1 1)
        (mesh-gen-box 3.4 0.2 10)
        (mesh-gen-box 10 0.2 3.4)
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
      (translate 2 -0.2 1))))

(define shadow-map-tex
  (make-instance texture2d '(512 512)
    (reserve 'rg-f32)))
(define shadow-map-depth-tex
  (make-instance texture2d '(512 512)
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
    (attach-texture 'color-buffer-0 shadow-map-tex)
    (attach-texture 'depth-buffer shadow-map-depth-tex)
    (set-visible-angle (/ pi 1.8))
    (set-depth-test #t)
    (set-bgcolor (make-fcolor 1 1 1 1))
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
($ (car (object-property shadow-map-display-rtask 'target)) : set-viewport
  (rect-from-size 200 200))

(define shadow-map-clear-rtask
  (rtask-def-clear light-cam '(depth-buffer color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define geometry-map-tex
  (make-instance texture2d '(800 600)
    (reserve 'rgba-f32)))

(define geometry-map-depth-tex
  (make-instance texture2d '(800 600)
    (reserve 'depth-f32)))

(define main-cam-transfrm
  (make-instance transfrm '()
    (translate 0 0 20)
    (rotate (/ pi 8) 'yOz)
    (rotate (/ pi 12) 'zOx)))

(define main-cam
  (make-instance camera '()
    (set-depth-test #t)
    (set-visible-angle (/ pi 5))
    (set-transformation main-cam-transfrm)
    (set-bgcolor (make-color #xff222222))))

(define geometry-map-shader
  (shader-from-config (load "geometry-map.scm")))

(define geometry-map-cam
  (make-instance camera '()
    (set-depth-test #t)
    (attach-texture 'color-buffer-0 geometry-map-tex)
    (attach-texture 'depth-buffer geometry-map-depth-tex)
    (set-bgcolor (color-from-rgba 0 0 0 0))))

(define geometry-map-rtasks
  (map (lambda (msh tf)
         (make-instance shading-rtask '()
           (set-attributes msh)
           (set-property-transfrm "transfrm" tf)
           (set-property-camera "camera" main-cam)
           (set-shader geometry-map-shader)
           (set-target geometry-map-cam)))
    meshes mesh-transfrms))

(define geometry-map-display-rtask
  (rtask-def-display-texture geometry-map-tex #:channel 'rgba))
($ (car (object-property geometry-map-display-rtask 'target)) : set-viewport
  (rect-from-size 200 150))

(define geometry-map-clear-rtask
  (rtask-def-clear geometry-map-cam '(depth-buffer color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define blur-temp-tex
  (make-instance texture2d '(512 512)
    (reserve 'rg-f32)))

(define blur-shader
  (shader-from-config (load "blur.scm")))

(define blur-quad
  (mesh-gen-plane 2 2 1 1))

(define propset-blur1
  (make-instance propset '()
    (append 5)
    (append 0)
    (append (make-mat 0.1 0.1 0.1 0.1 ':
                      0.1 0.0 0.0 0.0 ':
                      0.0 0.0 0.0 0.0 ':
                      0.0 0.0 0.0 0.0 ':))))

(define propset-blur2
  (make-instance propset '()
    (append 5)
    (append 1)
    (append (make-mat 0.1 0.1 0.1 0.1 ':
                      0.1 0.0 0.0 0.0 ':
                      0.0 0.0 0.0 0.0 ':
                      0.0 0.0 0.0 0.0 ':))))

(define target-blur1
  (make-instance camera '()
    (attach-texture 'color-buffer-0 blur-temp-tex)))

(define target-blur2
  (make-instance camera '()
    (attach-texture 'color-buffer-0 shadow-map-tex)))

(define blur1-rtask
  (make-instance shading-rtask '()
    (set-attributes blur-quad)
    (set-texture "texMap" shadow-map-tex)
    (set-property "blurParam" propset-blur1)
    (set-shader blur-shader)
    (set-target target-blur1)))

(define blur2-rtask
  (make-instance shading-rtask '()
    (set-attributes blur-quad)
    (set-texture "texMap" blur-temp-tex)
    (set-property "blurParam" propset-blur2)
    (set-shader blur-shader)
    (set-target target-blur2)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-shader
  (shader-from-config (load "blinn-phong-with-shadow.scm")))

(define propset-illum
  (make-instance propset '()
    (append-float 0.001)
    (append 1)
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
           (set-texture "geometryMap" geometry-map-tex)
           (set-shader main-shader)
           (set-target main-cam)))
    meshes mesh-transfrms))

(define main-clear-rtask
  (rtask-def-clear main-cam '(depth-buffer color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define main-rtask
  (make-instance queue-rtask '()
    (set-prof-enabled #t)

    (append shadow-map-clear-rtask)
    (append geometry-map-clear-rtask)
    (append main-clear-rtask)
    (append* shadow-map-rtasks)
    (append blur1-rtask)
    (append blur2-rtask)
    (append* geometry-map-rtasks)
    (append* screen-rtasks)
    (append shadow-map-display-rtask)))
    ;(append geometry-map-display-rtask)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; utility functions

(define rotate-light
  (lambda* (#:key (angle (/ pi 120)))
    ($- ($ light-cam : transformation) : rotate angle 'zOx)))

