(define-public shader-def-attr-mesh
  '(attributes
    (layout
      (col4 . "position")
      (col3 . "normal")
      (col3 . "uv"))))

(define-public shader-def-prop-transfrm
  '(property-group
    (name . "transfrm")
    (layout
      (mat4 . "mvpMatrix")
      (mat4 . "mMatrix"))))

