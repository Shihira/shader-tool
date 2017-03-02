(use-modules (shrtool))

(define perlin-mesh
  (mesh-gen-plane 2 2 1 1))

(define perlin-tex
  (make-instance texture2d '(512 512)
    (reserve 'r-f32)))

(define perlin-cam
  (make-instance camera '()
    (attach-texture 'color-buffer-0 perlin-tex)))

(define perlin-shader
  (shader-from-config (load "perlin.scm")))

(define perlin-rtask
  (make-instance shading-rtask '()
    (set-target perlin-cam)
    (set-attributes perlin-mesh)
    (set-shader perlin-shader)))

(define perlin-display-rtask
  (rtask-def-display-texture perlin-tex))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define cloud-mesh
  (mesh-gen-plane 1 1 256 256))

(define cloud-cam-transfrm
  (make-instance transfrm '()
     (rotate (/ pi 2) 'zOx)
     (translate 0 0.05 0.3)))

(define cloud-transfrm
  (make-instance transfrm '()))
(define cloud-transfrm-bottom
  (make-instance transfrm '()
    (rotate pi 'yOz)))

(define cloud-tex
  (make-instance texture2d '(800 600)
    (reserve 'r-f32)))

(define cloud-cam
  (make-instance camera '()
    (attach-texture 'color-buffer-0 cloud-tex)
    (set-draw-face 'both-face)
    (set-blend-func 'plus-blend)
    (set-bgcolor #xff000000)
    (set-near-clip-plane 0.0001)
    (set-far-clip-plane 1)
    (set-visible-angle (/ pi 2))
    (set-transformation cloud-cam-transfrm)))

(define cloud-shader
  (shader-from-config (load "volume-depth.scm")))

(define cloud-rtasks
  (map (lambda (msh tf)
         (make-instance shading-rtask '()
           (set-attributes msh)
           (set-property-transfrm "transfrm" tf)
           (set-property-camera "camera" cloud-cam)
           (set-texture "hfMap" perlin-tex)
           (set-shader cloud-shader)
           (set-target cloud-cam)))
    (list cloud-mesh cloud-mesh)
    (list cloud-transfrm cloud-transfrm-bottom)))

(define cloud-clear-rtask
   (rtask-def-clear cloud-cam '(color-buffer)))

(define cloud-display-rtask
   (rtask-def-display-texture cloud-tex #:channel 'r))
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define sky-shader
  (shader-from-config (load "sky.scm")))

(define sky-cam
  (make-instance camera '()
    (set-blend-func 'alpha-blend)
    (set-bgcolor #xffe0a553)))

(define sky-mesh
  (mesh-gen-plane 2 2 1 1))

(define sky-rtask
  (make-instance shading-rtask '()
    (set-shader sky-shader)
    (set-target sky-cam)
    (set-attributes sky-mesh)
    (set-texture "densityMap" cloud-tex)))

(define sky-clear-rtask
  (rtask-def-clear sky-cam '(color-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define cloud-travel
  (lambda ()
    ($- ($ cloud-cam : transformation) : rotate (/ pi -360) 'zOx)))

($ perlin-rtask : render)

(define main-rtask
  (make-instance queue-rtask '()
    (set-prof-enabled #t)
    (append cloud-clear-rtask)
    (append* cloud-rtasks)
    (append sky-clear-rtask)
    (append sky-rtask)))

