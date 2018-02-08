;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; clear

(define-public rtask-def-clear
  (lambda (rt bufs)
    (make-instance proc-rtask
      (list (lambda ()
              (letrec ((iter (lambda (x)
                               (if (pair? x)
                                 (begin
                                   ($ rt : clear-buffer (car x))
                                   (iter (cdr x)))
                                 #nil))))
                (iter bufs)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; display texture

(define-public shader-def-quad
  (lambda (property fragment-shader)
    `((name . "quad")
      ,shader-def-attr-mesh
      (property-group
        (name . "textures")
        (layout
         (tex2d . "texMap")))
      ,property
      (sub-shader
        (type . fragment)
        (source . ,(string-append "
          in vec2 texCoord;
          out vec4 outColor;
          " fragment-shader)))
      (sub-shader
        (type . vertex)
        (source . ,(string-append "
          out vec2 texCoord;
          vec2 procCoord(float x, float y) { return vec2(x, y); }
          void main() {
              gl_Position = vec4(position.z, position.x, 0, 1);
              texCoord = procCoord(position.z, position.x) / 2 + vec2(0.5, 0.5);
          }"))))))
    

(define display-texture-config
  (lambda (procColor procCoord)
  `((name . "solid-color")
    ,shader-def-attr-mesh
    (property-group
      (name . "textures")
      (layout
       (tex2d . "texMap")))
    (sub-shader
      (type . fragment)
      (source . ,(string-append "
        in vec2 texCoord;
        out vec4 outColor;
        
        vec4 procColor(vec4 c) {" procColor "}
        void main() {
            outColor = procColor(texture(texMap, texCoord));
        }")))
    (sub-shader
      (type . vertex)
      (source . ,(string-append "
        out vec2 texCoord;
        vec2 procCoord(float x, float y) {" procCoord "}
        void main() {
            gl_Position = vec4(position.z, position.x, 0, 1);
            texCoord = procCoord(position.z, position.x) / 2 + vec2(0.5, 0.5);
        }"))))))

(define-public rtask-def-display-texture
  (lambda* (tex #:key (upside-down #f) (channel 'rgba))
    (let* ((mesh (mesh-gen-plane 2 2 1 1))
           (channel-str (symbol->string channel))
           (channel-len (string-length channel-str))
           (procColor
             (case channel-len
               ((1) (string-append "float g = c." channel-str "; return vec4(g, g, g, 1); "))
               ((2) (string-append "return vec4(c." channel-str ", 0, 1);"))
               ((3) (string-append "return vec4(c." channel-str ", 1);"))
               ((4) (string-append "return c." channel-str ";"))))
           (procCoord
             (if upside-down "return vec2(x, -y);" "return vec2(x, y);"))
           (shader (shader-from-config
                     (display-texture-config procColor procCoord)))
           (cam (make-instance camera '()
                  (set-depth-test #f)))
           (rtask (make-instance shading-rtask '()
                    (set-attributes mesh)
                    (set-shader shader)
                    (set-target cam))))
      (if (string=? (instance-get-type tex #t) "image")
         ($ rtask : set-texture2d-image "texMap" tex)
         ($ rtask : set-texture "texMap" tex))
      (set-object-properties! rtask
        `((mesh ,mesh)
          (shader ,shader)
          (target ,cam)))
      rtask)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; display cubemap

(define display-cubemap-camera #nil)

(define-public rtask-def-display-cubemap-rotate
  (lambda* (#:optional (angle (/ pi -120)) (axis 'zOx))
    (if (not (null? display-cubemap-camera))
      ($- ($ display-cubemap-camera : transformation) : rotate angle axis)
      #nil)))

(define-public rtask-def-display-cubemap
  (lambda (tex)
    (let* ((shader 
             (built-in-shader "lambertian-cubemap"))
           (camera-transfrm
             (make-instance transfrm '()
               (translate 0 0 2)
               (rotate (/ pi 6) 'yOz)
               (rotate (/ pi 6) 'zOx)))
            (transfrm
              (make-instance transfrm '()))
            (mesh
              (mesh-gen-box 1 1 1))
            (illum
              (make-instance propset '()
                (append (mat-cvec 0 0 0 0))
                (append (make-color #xffffffff))))
            (material
              (make-instance propset '()
                (append-float 1)
                (append-float 0)))
            (camera
              (make-instance camera '()
                (set-bgcolor #xff222222)
                (set-transformation camera-transfrm)
                (set-depth-test #t)))
            (rtask
              (make-instance shading-rtask '()
                (set-property-transfrm "transfrm" transfrm)
                (set-attributes mesh)
                (set-property "illum" illum)
                (set-property "material" material)
                (set-property-camera "camera" camera)
                (set-target camera)
                (set-shader shader)
                (set-texture "texMap" tex)))
            (clr-rtask
               (rtask-def-clear camera '(color-buffer depth-buffer)))
            (q-rtask
               (make-instance queue-rtask '()
                 (append clr-rtask)
                 (append rtask))))
      (set-object-properties! q-rtask
        `((shader . ,shader)
          (transfrm . ,transfrm)
          (mesh . ,mesh)
          (illum . ,illum)
          (material . ,material)
          (camera . ,camera)
          (rtask . ,rtask)
          (clr-rtask . ,clr-rtask)))
      (set! display-cubemap-camera camera)
      q-rtask)))


