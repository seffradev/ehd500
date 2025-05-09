#let thesis(title, swedish_title, keywords: [], swedish_keywords:
[], authors: (), abstract: [],
swedish_abstract: [], preface: [], abbreviations: (), doc) = {
  set page(
    paper: "a4",
    margin: (x: 3cm, top: 5cm, bottom: 2cm),
    header: align(
      right + horizon,
      datetime.today().display()
    ),
  )
  set par(justify: true)
  set text(
    font: "Libertinus Serif",
    size: 12pt,
  )

  place(
    top + left,
    scope: "parent",
    float: true,
    dy: -100pt,
  )[
    #align(left + horizon,
      image("images/university-west-logo.jpg")
    )
  ]

  set align(left)
  text(28pt, title)

  set align(left)
  text(14pt, ..authors.map(author => [
    #author.name \
    #author.affiliation \
    #link("mailto:" + author.email)
  ]))

  align(bottom + left)[
    DEGREE PROJECT \
    Computer Engineering \
    Bachelor Level G2E, 15 hec \

    Department of Engineering Science, University West, Sweden
  ]

  pagebreak()
  
  counter(page).update(1)
  set page(
    header: align(
      center + horizon,
      text(14pt)[_EXAMENSARBETE_] + linebreak() + text(18pt,
      swedish_title) + line(length: 100%)
    ),
    numbering: "i",
  )

  heading([Sammanfattning], outlined: false)

  swedish_abstract

  align(bottom + center, table(
    columns: (1fr, auto),
    stroke: none,
    align: top + left,
    [*Datum:*], datetime.today().display(),
    [*Författare:*], text(..authors.map(author => [ #author.name ])),
    [*Examinator:*], [Stanislav Belenki],
    [*Handledare:*], [
      Hanna Aknouche-Martinsson, Högskolan Väst och \
      Mikael Johansson, Leissner Data AB
    ],
    [*Program:*], [
      Datateknik, högskoleingenjör - programmering och \
      nätverksteknik, 180hp
    ],
    [*Huvudområde:*], [Datateknik],
    [*Utbildningsnivå:*], [Grundnivå],
    [*Kurskod:*], [EHD500, 15hp],
    [*Nyckelord:*], swedish_keywords,
    [*Utgivare:*], [
      Institutionen för ingenjörsvetenskap, Högskolan Väst \
      461 86 Trollhättan
    ],
  ))

  pagebreak()

  set page(
    header: align(
      center + horizon,
      text(14pt)[_DEGREE PROJECT_] + linebreak() + text(18pt,
      title) + line(length: 100%)
    ),
  )

  heading([Summary], outlined: false)

  abstract

  align(bottom + center, table(
    columns: (1fr, auto),
    stroke: none,
    align: top + left,
    [*Date:*],
    datetime.today().display("[day] [month repr:long] [year]"),
    [*Author:*], text(..authors.map(author => [ #author.name ])),
    [*Examiner:*], [Stanislav Belenki],
    [*Advisor:*], [
      Hanna Aknouche-Martinsson, University West and \
      Mikael Johansson, Leissner Data AB
    ],
    [*Programme:*], [
      Computer Engineering -- programming and \
      network technology, 180hp
    ],
    [*Main field of study:*], [Computer engineering],
    [*Education level:*], [First cycle],
    [*Course code:*], [EHD500, 15 HE credits],
    [*Keywords:*], keywords,
    [*Publisher:*], [
      Department of Engineering Science, University West \
      SE-461 86 Trollhättan, Sweden
    ],
  ))

  set page(
    header: align(
      center + horizon,
      emph(title) + line(length: 100%)
    ),
  )

  pagebreak()

  heading([Preface], outlined: false)
  preface

  pagebreak()

  outline(title: [Table of contents], depth: 2)

  if abbreviations.len() > 1 {
    pagebreak()
    heading([Abbreviations])
    align(center, table(
      align: top + left,
      stroke: none,
      columns: (1fr, 3fr),
      ..for(short, long) in abbreviations {
        (table.cell(short), table.cell(long))
      }
    ))
  }

  set heading(numbering: "1.")
  counter(page).update(1)
  set page(
    numbering: "1",
  )

  set align(left)
  show heading.where(level: 1): it => pagebreak() + text(it)
  show heading.where(level: 4): it => {
    if (it.level >= 3) {
      block(it.body)
    } else {
      block(counter(heading).display() + " " + it.body)
    }
  }

  doc

  bibliography("references.bib", title: "References")
}
