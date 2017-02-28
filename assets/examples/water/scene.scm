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
    (attach-texture 'color-buffer-0 (cadr hf-texs))
    (set-depth-test #f)))

(define hf-rtask
  (make-instance shading-rtask '()
    (set-target hf-cam)
    (set-shader hf-shader)
    (set-texture "hfMap" (car hf-texs))
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
    (translate 0 -0.3 3)
    (rotate (/ pi 8) 'yOz)
    (rotate (/ pi 1.1) 'zOx)))

(define screen-cam
  (make-instance camera '()
    (set-bgcolor #xff222222)
    (set-depth-test #t)
    (set-visible-angle (/ pi 8.4))
    (set-draw-face 'back-face)
    (set-transformation cam-transfrm)))

(define screen-clear-rtask
  (rtask-def-clear screen-cam '(depth-buffer color-buffer)))

(define water-surf-skybox-image
  ($ (built-in-image "skybox-texture-small-2") : extract-cubemap))

(define env-map-cubemap
  (make-instance texture-cubemap '(512)
    (reserve 'rgba-u8888)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; water surface rendering

(define water-surf-plane
  (mesh-gen-plane 1 1 (- hf-grids 3) (- hf-grids 3)))

(define water-surf-transfrm
  (make-instance transfrm '()))

(define water-surf-propset-illum
  (make-instance propset '()
    (append (mat-cvec -4 1 -2 1))
    (append (make-color #xffffffff))))

(define water-surf-shader
  (shader-from-config (load "water-surface.scm")))

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
    ;(set-texture-cubemap-image "envMap" water-surf-skybox-image)
    (set-texture-cubemap-image "skyboxMap" water-surf-skybox-image)
    (set-texture "envMap" env-map-cubemap)
    (set-shader water-surf-shader)))

(define pre-water-surf-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         ($ water-surf-rtask : set-texture "hfMap" (cadr hf-texs))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; caustics rendering

(define caustics-tex
  (make-instance texture2d '(1024 1024)
    (reserve 'r-f32)))

(define caustics-shader
  (shader-from-config (load "caustics.scm")))

(define caustics-cam
  (make-instance camera '()
    (set-depth-test #f)
    (set-bgcolor #xff000000)
    (set-near-clip-plane 0.2)
    (set-far-clip-plane 1.5)
    (set-visible-angle (/ pi 2))
    (set-blend-func 'plus-blend)
    (attach-texture 'color-buffer-0 caustics-tex)))

($ ($ caustics-cam : transformation) : translate 0 0 0.5)
($ ($ caustics-cam : transformation) : rotate (/ pi 2) 'yOz)

(define caustics-rtask
  (make-instance shading-rtask '()
    (set-shader caustics-shader)
    (set-attributes water-surf-plane)
    (set-property-camera "camera" caustics-cam)
    (set-property "illum" water-surf-propset-illum)
    (set-target caustics-cam)))

(define pre-caustics-rtask
  (make-instance proc-rtask
    `(,(lambda ()
         ($ caustics-rtask : set-texture "hfMap" (cadr hf-texs))))))

(define caustics-display-rtask
  (rtask-def-display-texture caustics-tex #:channel 'r))
(define caustics-clear-rtask
  (rtask-def-clear caustics-cam '(color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pool 

(define wall-propset-material
  (make-instance propset '()
    (append (make-color #xfff5cf83))))

(define wall-mesh
  (mesh-gen-box 1 1 1))

(define wall-transfrm
    (make-instance transfrm '()))

(define wall-shader
  (shader-from-config ((load "pool-wall.scm") #f)))

(define wall-rtask
   (make-instance shading-rtask '()
     (set-target screen-cam)
     (set-attributes wall-mesh)
     (set-shader wall-shader)
     (set-texture "causticsMap" caustics-tex)
     (set-property "material" wall-propset-material)
     (set-property "illum" water-surf-propset-illum)
     (set-property-transfrm "transfrm" wall-transfrm)
     (set-property-camera "camera" screen-cam)
     (set-property-camera "causticsCamera" caustics-cam)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; render to cubemap

(define env-map-cubemap-depth-buffer
  (make-instance texture-cubemap '(512)
    (reserve 'depth-f32)))

(define env-map-cam
  (make-instance camera '()
    (set-draw-face 'both-face)
    (set-bgcolor #xff000000)
    (set-near-clip-plane 0.1)
    (set-far-clip-plane 10)
    (attach-texture 'color-buffer-0 env-map-cubemap)
    (attach-texture 'depth-buffer env-map-cubemap-depth-buffer)))

(define env-map-shader
  (shader-from-config ((load "pool-wall.scm") #t)))

(define env-map-clear-rtask
  (rtask-def-clear env-map-cam '(color-buffer depth-buffer)))

(define env-map-rtask
  (make-instance shading-rtask '()
    (set-target env-map-cam)
    (set-attributes wall-mesh)
    (set-shader env-map-shader)
    (set-property-camera "camera" env-map-cam)
    (set-property "material" wall-propset-material)
    (set-property "illum" water-surf-propset-illum)
    (set-texture "causticsMap" caustics-tex)
    (set-property-transfrm "transfrm" wall-transfrm)
    (set-property-camera "causticsCamera" caustics-cam)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define display-cubemap-camera-transfrm
  (make-instance transfrm '()
    (translate 0 0 2)
    (rotate (/ pi -6) 'yOz)
    (rotate (/ pi 6) 'zOx)))

(define display-cubemap-transfrm
  (make-instance transfrm '()))

(define display-cubemap-mesh
  (mesh-gen-box 1 1 1))

(define display-cubemap-illum
  (make-instance propset '()
    (append (mat-cvec 0 0 0 0))
    (append (make-color #xffffffff))))

(define display-cubemap-material
  (make-instance propset '()
    (append-float 1)
    (append-float 0)))

(define display-cubemap-camera
  (make-instance camera '()
    (set-bgcolor #xff222222)
    (set-transformation display-cubemap-camera-transfrm)
    (set-depth-test #t)))

(define display-cubemap-rotate
  (lambda ()
    ($- ($ display-cubemap-camera : transformation) : rotate (/ pi -120) 'zOx)))

(define display-cubemap-shader
  (built-in-shader "lambertian-cubemap"))

(define display-cubemap-clear-rtask
  (rtask-def-clear display-cubemap-camera '(color-buffer depth-buffer)))

(define display-cubemap-rtask
  (make-instance shading-rtask '()
    (set-property-transfrm "transfrm" display-cubemap-transfrm)
    (set-attributes display-cubemap-mesh)
    (set-property "illum" display-cubemap-illum)
    (set-property "material" display-cubemap-material)
    (set-property-camera "camera" display-cubemap-camera)
    (set-target display-cubemap-camera)
    (set-shader display-cubemap-shader)
    (set-texture "texMap" env-map-cubemap)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; main-rtask

(define main-rtask
  (make-instance queue-rtask '()
    (set-prof-enabled #t)
    (append screen-clear-rtask)

    (append pre-hf-rtask)
    (append hf-rtask)
   ;(append pre-hf-texdisp-rtask)
   ;(append hf-texdisp-rtask)))

    (append pre-caustics-rtask)
    (append caustics-clear-rtask)
    (append caustics-rtask)
   ;(append caustics-display-rtask)))

    (append env-map-clear-rtask)
    (append env-map-rtask)
   ;(append display-cubemap-clear-rtask)
   ;(append display-cubemap-rtask)))

    (append wall-rtask)
    (append pre-water-surf-rtask)
    (append water-surf-rtask)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; utility functions

(import (srfi srfi-27))

(define drop
  (lambda* (#:optional
             (x (random-integer hf-grids))
             (y (random-integer hf-grids))
            #:key (r (* 5 pi)) (s 5))
    ($ dist-propset : set 0 2)
    ($ dist-propset : set 1 (mat-cvec x y))
    ($ dist-propset : set-float 2 r)
    ($ dist-propset : set-float 3 s)
    ($ pre-dist-rtask : render)
    ($ dist-rtask : render)))

(define reset-water
  (lambda ()
    ($ dist-propset : set 0 1)
    ($ pre-dist-rtask : render)
    ($ dist-rtask : render)))

(define rotate-camera
  (lambda* (#:key (angle (/ pi 120)) (dir 'zOx))
    ($- ($ screen-cam : transformation) : rotate angle dir)))

