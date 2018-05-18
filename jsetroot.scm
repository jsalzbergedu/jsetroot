@(heading "Introduction")
@(text "{{Jsetroot}} is a library for accessing the functionality of
[[https://github.com/elmiko/hsetroot|hsetroot]] from scheme. Most importantly,
it can be used to create animations, unlike hsetroot, which creates a new
process every time a new image is made.")

@(heading "Documentation")
@(noop)

(module jsetroot
    (export get-display! set-background-image! xopendisplay)
  (import chicken scheme foreign)
  (require-extension xlib)
  (reexport xlib)
  (foreign-declare "#include \"hsetroot.c\"")
  (define (get-display! k)
    @("Pass k a pointer to an opened x display."
      "It is not safe to bind this value with define."
      (k "A continuation to which the pointer to the display is passed.")
      (@example-no-eval "Pass the pointer to a continuation"
                        (get-display! (lambda (pointer)
                                        (print "Here's my pointer! "
                                               pointer)))))
    (let ((display (xopendisplay #f)))
      (k display)))
  (define c-set-background-image!
    (foreign-lambda void "set_background_image" nonnull-c-string
                    c-pointer))
  (define (set-background-image! filename display)
    @("Calls into a foreign function to set the background image."
      (filename "The filename of the background image")
      (display "A pointer to an x display")
      (@example-no-eval "Set the background image to kitty.png"
                        (get-display! (lambda (display)
                                        (set-background-image! filename
                                                               display)))))
    (c-set-background-image! filename display)))
