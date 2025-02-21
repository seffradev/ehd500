pdflatex --shell-escape main
biber main
pdflatex --shell-escape main

pdflatex --shell-escape proposal
biber proposal
pdflatex --shell-escape proposal
