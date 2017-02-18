(define-public shader-def-attr-mesh
  '(attributes
    (layout
      (col4 . "position")
      (col3 . "normal")
      (col3 . "uv"))))

(define-public shader-def-prop-camera
  (lambda* (#:key (name "camera") (prefix ""))
    `(property-group
      (name . ,name)
      (layout
        (mat4 . ,(string-append prefix "vMatrix"))
        (mat4 . ,(string-append prefix "vMatrix_inv"))
        (mat4 . ,(string-append prefix "vpMatrix"))))))

(define-public shader-def-prop-transfrm
  (lambda* (#:key (name "transfrm") (prefix ""))
    `(property-group
      (name . ,name)
      (layout
        (mat4 . ,(string-append prefix "mMatrix"))
        (mat4 . ,(string-append prefix "mMatrix_inv"))))))

