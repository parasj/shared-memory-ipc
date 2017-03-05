(TeX-add-style-hook
 "report"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("scrartcl" "paper=a4" "fontsize=11pt")))
   (TeX-add-to-alist 'LaTeX-provided-package-options
                     '(("fontenc" "T1") ("babel" "english")))
   (TeX-run-style-hooks
    "latex2e"
    "scrartcl"
    "scrartcl10"
    "fontenc"
    "fourier"
    "graphicx"
    "booktabs"
    "tabularx"
    "babel"
    "amsmath"
    "amsfonts"
    "amsthm"
    "lipsum"
    "sectsty"
    "fancyhdr")
   (TeX-add-symbols
    '("horrule" 1)))
 :latex)

