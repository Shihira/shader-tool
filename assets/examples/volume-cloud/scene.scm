(use-modules (shrtool))

(define fbm-mesh
  (mesh-gen-plane 2 2 1 1))

(define fbm-tex
  (make-instance texture2d '(512 512)
    (reserve 'rgba-f32)))

(define fbm-cam
  (make-instance camera '()
    (attach-texture 'color-buffer-0 fbm-tex)))

(define fbm-shader
  (shader-from-config (load "fbm.scm")))

(define fbm-propset
  (make-instance propset '()
    (append-float 0)
    (append-float 0)
    (append-float 0.5)
    (append (mat-cvec -5 6 -4))))

(define fbm-rtask
  (make-instance shading-rtask '()
    (set-target fbm-cam)
    (set-attributes fbm-mesh)
    (set-property "prop" fbm-propset)
    (set-shader fbm-shader)))

(define fbm-display-rtask
  ;(rtask-def-display-texture fbm-tex #:channel 'r))
  (rtask-def-display-texture fbm-tex))

(define main-rtask
  (make-instance queue-rtask '()
    (append fbm-rtask)
    (append fbm-display-rtask)))

(define param '(0 0 0.5))

(define inc
  (lambda* (#:optional (pos 0))
    (let ((val (+ (list-ref param pos) 0.01)))
      (list-set! param pos val)
      ($ fbm-propset : set-float pos val))))

(define dec
  (lambda* (#:optional (pos 0))
    (let ((val (- (list-ref param pos) 0.01)))
      (list-set! param pos val)
      ($ fbm-propset : set-float pos val))))

