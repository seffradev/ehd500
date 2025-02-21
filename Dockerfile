FROM texlive/texlive AS development

RUN apt update
RUN apt install inkscape -y

WORKDIR /docs

COPY docs/whitepaper .

ENTRYPOINT [ "sh", "-c", "entrypoint.sh" ]
