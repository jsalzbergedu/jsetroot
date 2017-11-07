(module jsetroot
    (export get-display! set-background-image! xopendisplay)
  (import chicken scheme foreign)
  (require-extension xlib)
  (reexport xlib)
  (foreign-declare "#include \"hsetroot.c\"")
  (define (get-display! k) (let ((display (xopendisplay #f)))
			     (k display)))
  (define set-background-image!
    (foreign-lambda void "set_background_image" nonnull-c-string c-pointer)))
