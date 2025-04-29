set -xe

for image in images/*.mmd
do
	node mermaid-cli/src/cli.js -i $image --outputFormat=png --iconPacks "@iconify-json/solar" --width=3200 --height=3200 
done

typst compile thesis.typ thesis.pdf
