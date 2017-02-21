(define-module (shrtool))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
               (number->string (length args))) args)))
      (interaction-environment))))

(load-extension "libshrtool" "shrtool_init_scm")

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

(define symbol-concat
  (lambda (a . b)
    (letrec ((f (lambda (s tail)
                  (if (null? tail) s
                    (f (string-append s
                         (symbol->string (car tail))) (cdr tail))))))
      (string->symbol (f (symbol->string a) b)))))

(define-syntax apply-member-func
  (lambda (x)
    (syntax-case x ()
      ((_ type inst (func param ...))
        #'($ inst : func param ...)))))
(export apply-member-func)

(define-syntax make-instance
  (lambda (x)
    (syntax-case x ()
      ((_ type ctor-param prop ...)
       (with-syntax
         ((ctor (datum->syntax x
                  (symbol-concat 'make- (syntax->datum #'type)))))
         #'(let ((inst (apply ctor ctor-param)))
             (apply-member-func type inst prop) ...
             inst))))))
(export make-instance)

(define-syntax $
  (lambda (x)
    (syntax-case x ()
      (($ inst : func param ...)
       (with-syntax
         ((func-str (datum->syntax x
                      (symbol->string (syntax->datum #'func)))))
         #'((instance-search-function inst func-str) inst param ...))))))
(export $)

(define-syntax $-
  (lambda (x)
    (syntax-case x ()
      (($- inst : func param ...)
       #'(begin ($ inst : func param ...) #nil)))))
(export $-)

(load "shader-def.scm")
(load "rtask-def.scm")
(load "matrix.scm")
(load "zenity-extension.scm")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
    (let* ((stack #nil)
           (val
            (catch #t
              (lambda () (eval-body str))
              (lambda (k . a)
                (cond
                  ((eq? k 'read-error) #nil)
                  ((eq? k 'quit) #nil)
                  (else
                    (display "Uncaught exception: ")
                    (display (cons k a))
                    (newline)))
                   ;(newline)
                   ;(if (not (null? stack))
                   ;  (display-backtrace stack (current-error-port) 2)
                   ;  (begin))))
                (list #f k a))
              (lambda (k . a)
                (set! stack (make-stack #t)))))
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (srfi srfi-1))

(define-public queue-rtask-append*
  (lambda (qrtask rtasks)
    (cond
      ((and (list? rtasks)) ; recusively append
       (fold (lambda (cur _)
               (queue-rtask-append qrtask cur))
         #nil rtasks))
      (else (queue-rtask-append qrtask rtasks)))))

