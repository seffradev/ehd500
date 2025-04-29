set -xe

pdflatex --shell-escape halftime-presentation
biber halftime-presentation
pdflatex --shell-escape halftime-presentation

pdflatex --shell-escape proposal
biber proposal
pdflatex --shell-escape proposal
