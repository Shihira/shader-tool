(import (ice-9 popen))
(import (ice-9 rdelim))
(import (ice-9 regex))

(define-public select-color
  (lambda ()
    (let* ((pipe (open-input-pipe "zenity --color-selection 2>/dev/null"))
           (str (read-line pipe)) ;"rgb(???,???,???)"
           (str-no-semi
             (regexp-substitute/global #f "," str 'pre " " 'post))
           (arg-list (read (open-input-string 
                             (string-append "(" str-no-semi ")"))))
           (color-list
             (if (eq? (car arg-list) 'rgb)
               (append (cadr arg-list) '(255))
               (append (list-head (cadr arg-list) 3)
                 `(,(* 255 (list-ref (cadr arg-list) 3)))))))
     (apply color-from-rgba color-list))))

(define-public select-file
  (lambda ()
    (let* ((pipe (open-input-pipe "zenity --file-selection 2>/dev/null")))
      (read-line pipe))))

