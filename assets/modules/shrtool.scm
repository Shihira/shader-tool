(define-module (shrtool))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define list-length
  (lambda (l)
    (cond
      ((null? l) 0)
      (#t (+ 1 (list-length (cdr l)))))))

(define-public shrtool-register-function
  (lambda (func typenm funcnm)
    (eval
      `(define-public ,func
         (lambda args
           (general-subroutine ,typenm ,funcnm args)))
      (interaction-environment))))

(define-public shrtool-register-constructor
  (lambda (funcnm typenm)
    (eval
      `(define-public ,funcnm
         (lambda args
           (general-subroutine ,typenm
             (string-append "__init_"
               (number->string (list-length args))) args)))
      (interaction-environment))))

(load-extension "libshrtool" "shrtool_init_scm")

(load "shader-def.scm")
(load "matrix.scm")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public built-in-shader
  (lambda (name)
    (shader-from-config
      (load (string-append
              (getenv "SHRTOOL_ASSETS_DIR")
              file-name-separator-string
              "shaders"
              file-name-separator-string
              name
              ".scm")))))

(define-public built-in-model
  (lambda (name)
    (meshes-from-wavefront
      (string-append
        (getenv "SHRTOOL_ASSETS_DIR")
        file-name-separator-string
        "models"
        file-name-separator-string
        name
        ".obj"))))

(define-public built-in-image
  (lambda (name)
    (image-from-ppm
      (string-append
        (getenv "SHRTOOL_ASSETS_DIR")
        file-name-separator-string
        "textures"
        file-name-separator-string
        name
        ".ppm"))))

(define eval-counter 1)

(define find-var ;eval in interaction-environment
  (lambda (obj)
    (module-ref (current-module) obj)))

(define eval-body
  (lambda (str)
    (let* ((org-sexpr (read (open-input-string str))) ; read the first sexpr
           (full-sexpr (if (and (symbol? org-sexpr)
                                (defined? org-sexpr)
                                (or
                                  (procedure? (find-var org-sexpr))
                                  (macro? (find-var org-sexpr))))
                         (read (open-input-string
                                 (string-append "(" str ")")))
                         org-sexpr)))
      (cons #t (eval full-sexpr (interaction-environment))))))

(define-public shrtool-repl-body
  (lambda (str)
    (let* ((val
            (catch #t
              (lambda () (eval-body str))
              (lambda (k . a)
                (cond
                  ((eq? k 'read-error) #f)
                  ((eq? k 'quit) #f)
                  (else
                    (display "Uncaught exception: ")
                    (display (cons k a))
                    (newline)))
                (list #f k a))))
           (suc (car val))
           (ret (cdr val))
           (varname (string-append "$" (number->string eval-counter))))
      (if (not (or
                 (not suc)
                 (null? ret)
                 (eof-object? ret)
                 (unspecified? ret)))
        (begin
          (display (string-append varname " = "))
          (display ret)
          (newline)
          (eval
            (list 'define (string->symbol varname) ret)
            (interaction-environment))
          (set! eval-counter (+ 1 eval-counter))
          val)
        val))))

