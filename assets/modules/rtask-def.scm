(define-public rtask-def-clear
  (lambda (rt bufs)
    (cons
      (make-instance proc-rtask
        (list (lambda ()
                (letrec ((iter (lambda (x)
                                 (if (pair? x)
                                   (begin
                                     ($ rt : clear-buffer (car x))
                                     (iter (cdr x)))
                                   #nil))))
                  (iter bufs))))) '())))

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
      (version . "330 core")
      (source . ,(string-append "
        in vec2 texCoord;
        out vec4 outColor;
        
        vec4 procColor(vec4 c) {" procColor "}
        void main() {
            outColor = procColor(texture(texMap, texCoord));
        }")))
    (sub-shader
      (type . vertex)
      (version . "330 core")
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
      (cons rtask
        `((mesh ,mesh)
          (shader ,shader)
          (target ,cam))))))

