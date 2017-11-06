#>
#include "hsetroot.c"
<#
(define set-background-image (foreign-lambda void "set_background_image" nonnull-c-string))
(set-background-image "/home/jacob/.stumpwm.d/data/frames/1.png")
