set -xe

for image in images/*.mmd
do
	node mermaid-cli/src/cli.js -i $image --outputFormat=png --iconPacks "@iconify-json/solar" --width=3200 --height=3200 
done

pdflatex --shell-escape main
biber main
pdflatex --shell-escape main

pdflatex --shell-escape proposal
biber proposal
pdflatex --shell-escape proposal
