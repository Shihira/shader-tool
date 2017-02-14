(define-public shader-def-attr-mesh
  '(attributes
    (layout
      (col4 . "position")
      (col3 . "normal")
      (col3 . "uv"))))

(define-public shader-def-prop-camera
  '(property-group
    (name . "camera")
    (layout
      (mat4 . "vMatrix")
      (mat4 . "vpMatrix"))))

(define-public shader-def-prop-transfrm
  '(property-group
    (name . "transfrm")
    (layout
      (mat4 . "mMatrix")
      (mat4 . "mMatrix_inv"))))

