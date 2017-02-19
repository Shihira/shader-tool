(import (shrtool))

(define standard-panel
  (mesh-gen-plane 2 2 1 1))

(define hf-grids 256)

(define hf-texs                             ; after calling (use-hf-tex)
  (list (make-instance texture2d (list hf-grids hf-grids)
          (reserve 'rg-f32))               ; prepared height field map
        (make-instance texture2d (list hf-grids hf-grids)
          (reserve 'rg-f32))))              ; map to render

(define use-hf-tex
  (lambda ()
    (set! hf-texs (list (cadr hf-texs) (car hf-texs)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; height field rendering

(define hf-shader
  (shader-from-config (load "height-fields.scm")))

(define hf-cam
  (make-instance camera '()
    (set-depth-test #f)))

(define hf-rtask
  (make-instance shading-rtask '()
    (set-target hf-cam)
    (set-shader hf-shader)
    (set-attributes standard-panel)))

(define pre-hf-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         (use-hf-tex)
         ($ hf-cam : attach-texture 'color-buffer-0 (cadr hf-texs))
         ($ hf-rtask : set-texture "hfMap" (car hf-texs))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; distrubance rendering

(define dist-shader
  (shader-from-config (load "hf-disturbance.scm")))

(define dist-propset
  (make-instance propset '()
    (append 0)
    (append (mat-cvec 0 0))
    (append-float 0)
    (append-float 0)))

(define dist-cam
  (make-instance camera '()
    (set-bgcolor #xff000000)
    (set-depth-test #f)))

(define dist-rtask
  (make-instance shading-rtask '()
    (set-target dist-cam)
    (set-shader dist-shader)
    (set-property "actions" dist-propset)
    (set-attributes standard-panel)))

(define pre-dist-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         (use-hf-tex)
         ($ dist-cam : attach-texture 'color-buffer-0 (cadr hf-texs))
         ($ dist-rtask : set-texture "hfMap" (car hf-texs))))))

; got textures prepared
($ pre-dist-rtask : render)
($ dist-cam : clear-buffer 'color-buffer)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define hf-texdisp-rtask
  (rtask-def-display-texture (cadr hf-texs) #:channel 'r))

(define pre-hf-texdisp-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         ($ (car hf-texdisp-rtask) : set-texture "texMap" (cadr hf-texs))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define cam-transfrm
  (make-instance transfrm '()
    (translate 0 0 3)
    
    (rotate (/ pi 8) 'yOz)(rotate (/ pi 6) 'zOx)))

(define screen-cam
  (make-instance camera '()
    (set-bgcolor #xff333333)
    (set-depth-test #t)
    (set-visible-angle (/ pi 8.4))
    (set-transformation cam-transfrm)))

(define screen-clear-rtask
  (rtask-def-clear screen-cam '(depth-buffer color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; water surface rendering

(define water-surf-env-map
  ($ (built-in-image "skybox-texture-small") : extract-cubemap))

(define water-surf-plane
  (mesh-gen-plane 1 1 hf-grids hf-grids))

(define water-surf-transfrm
  (make-instance transfrm '()))

(define water-surf-shader
  (shader-from-config (load "water-surface.scm")))

(define water-surf-propset-illum
  (make-instance propset '()
    (append (mat-cvec 4 2 4 1))
    (append (make-color #xffffffff))))

(define water-surf-propset-material
  (make-instance propset '()
    (append-float 0.0)
    (append-float 1.0)))

(define water-surf-rtask
  (make-instance shading-rtask '()
    (set-attributes water-surf-plane)
    (set-target screen-cam)
    (set-property "illum" water-surf-propset-illum)
    (set-property "material" water-surf-propset-material)
    (set-property-transfrm "transfrm" water-surf-transfrm)
    (set-property-camera "camera" screen-cam)
    (set-texture-cubemap-image "envMap" water-surf-env-map)
    (set-shader water-surf-shader)))

(define pre-water-surf-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         ($ water-surf-rtask : set-texture "hfMap" (cadr hf-texs))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; main-rtask

(define main-rtask
  (make-instance queue-rtask '()
    (append pre-hf-rtask)
    (append hf-rtask)
    ;(append pre-hf-texdisp-rtask)
    ;(append (car hf-texdisp-rtask))))
    (append (car screen-clear-rtask))
    (append pre-water-surf-rtask)
    (append water-surf-rtask)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; utility functions

(import (srfi srfi-27))

(define drop
  (lambda* (#:optional
             (x (random-integer hf-grids))
             (y (random-integer hf-grids))
            #:key (r 10) (s 8))
    ($ dist-propset : set 0 2)
    ($ dist-propset : set 1 (mat-cvec x y))
    ($ dist-propset : set-float 2 r)
    ($ dist-propset : set-float 3 s)
    ($ pre-dist-rtask : render)
    ($ dist-rtask : render)))

(define rotate-camera
  (lambda* (#:key (angle (/ pi 120)) (dir 'zOx))
    ($- ($ screen-cam : transformation) : rotate angle dir)))

