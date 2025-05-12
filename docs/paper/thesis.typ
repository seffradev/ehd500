#import "template.typ": thesis

#show: thesis.with(
  [
    Physically separated SIM- and UE-connectivity device
    prototypes
  ],
  [
    Fysiskt separerade SIM- och UE-konnektivitetsenhetsprototyper
  ],
  authors: (
    (
      name: "Hampus Avekvist",
      affiliation: "University West",
      email: "hampus.avekvist@hey.com",
    ),
  ),
  abstract: [
    This paper introduces the concept of Subscriber Identity Module
    (SIM) banks, User Equipment (UE) sinks and discuss a mediator
    of these. The thesis itself doesn't design the mediator but
    presents it as the end-goal of the work the thesis starts.

    Communication between the banks and sinks are discussed where
    specifically forwarding of Application Protocol Data Units
    (APDU) is highlighted as an essential function to be
    implemented.

    A prototype-based software engineering approach is employed in
    designing two devices based on Raspberry Pi, SIMtrace 2 and
    smart card reader hardware. The two devices implement a
    protocol for the forwarding of APDU packets.

    A thorough discussion on future work is presented where
    experimentation and requirements elicitation is highlighted. As
    those parts have been conducted, it's suggested that a full
    system implementation is written.
  ],
  swedish_abstract: [
    Denna uppsats introducerar begreppen Subscriber Identity Module
    (SIM)-banker, User Equipment (UE)-mottagare och tar upp om en
    medlare av dessa. Tesen i sig gör ingen design av medlaren men
    presenterar den som slutmålet för arbetet tesen startar.

    Kommunikation mellan bankerna och mottagarna är fördjupade inom
    där Application Protocol Data Unit (APDU)-vidaresändning lyfts
    som en väsentlig funktion.

    Ett prototypbaserat tillvägagångssätt till mjukvaruutveckling
    används för att designa två Raspberry Pi-, SIMtrace 2- och
    smart card-läsarbaserade enheter. Dessa enheter implementerar
    ett protokoll för vidaresändning av APDU-paket.

    En utförlig diskussion om framtida arbeten presenteras där
    experimentering och kravinsamling lyfts. När de delarna har
    utförts föreslås utförandet av en fullständig
    systemimplementation.
  ],
  keywords: [
    Telecommunications, requirements elicitation, coverage testing,
    #linebreak() protocol design, PC/SC,
  ],
  swedish_keywords: [
    Telekommunikation, kravinsamling, täckningstestning,
    #linebreak() protokolldesign, PC/SC,
  ],
  preface: [
    First, I'd like to thank my classmates whom never stumbled in
    their trust and belief that I'd make it to the other side of
    this thesis.

    Second, I'd like to thank Leissner Data AB for the opportunity
    to implement a novel technical system where much creativity had
    to be employed. I would also like to thank them for the
    independence and trust to work in my own way, allowing a
    thorough attempt at using the skills taught during the
    computer engineering program. I'm giving a specific thanks to
    Peter Fässberg for the depths of knowledge he provided during
    the design and discussions of the prototype.

    And last, I'd like to thank Hanna Aknouche-Martinsson for her
    supervision. Without her, the project would be out of scope and
    never find its way to completion in time. She helped me in
    deciding what to keep, what to remove and what to change.

    Trollhättan, May 2025
    #linebreak()
    Hampus Avekvist
    ],
  abbreviations: (
    (
      short: "2G",
      long: "Second generation telecommunications technology",
    ),
    (
      short: "3G",
      long: "Third generation telecommunications technology",
    ),
    (
      short: "4G",
      long: "Fourth generation telecommunications technology",
    ),
    (
      short: "5G",
      long: "Fifth generation telecommunications technology",
    ),
    (
      short: "SIM",
      long: "Subscriber Identity Module",
    ),
    (
      short: "USIM",
      long: "Universal Subscriber Identity Module",
    ),
    (
      short: "UE",
      long: "User Equipment",
    ),
    (
      short: "APDU",
      long: "Application Protocol Data Unit",
    ),
    (
      short: "PC/SC",
      long: "Personal Computer/Smart Card",
    ),
    (
      short: "ISP",
      long: "Internet Service Provider",
    ),
    (
      short: "PGW",
      long: "Packet Delivery Network Gateway",
    ),
    (
      short: "SMSC",
      long: "Short Message Service Center",
    ),
    (
      short: "HSS",
      long: "Home Subscriber Service",
    ),
    (
      short: "MVNE",
      long: "Mobile Virtual Network Enabler",
    ),
    (
      short: "MVNO",
      long: "Mobile Virtual Network Operator",
    ),
    (
      short: "IMSI",
      long: "International Mobile Subscriber Identifier",
    ),
    (
      short: "SPN",
      long: "Service Provider Name",
    ),
    (
      short: "PIN",
      long: "Personal Identification Number",
    ),
    (
      short: "ICCID",
      long: "Integrated Circuit Card Identifier",
    ),
    (
      short: "ATR",
      long: "Answer To Reset",
    ),
    (
      short: "ISO",
      long: "International Organization for Standardization",
    ),
    (
      short: "TCP",
      long: "Transmission Control Protocol",
    ),
    (
      short: "ATP",
      long: "APDU and ATP Tunneling Protocol",
    ),
    (
      short: "ICCID",
      long: "Integrated Circuit Card Identifier",
    ),
    (
      short: "TARA",
      long: "Threat Assessment and Remediation Analysis",
    ),
  ),
)

= Introduction <sec:introduction>

Connectivity is the backbone for modern society. The ability to
keep in touch, share knowledge and collaborate remotely is growing
more important. A group of technologies and infrastructure enables
these facets, all under the telecommunications umbrella.

For a user of these technologies and infrastructure, identification
is required to ascertain the authenticity and capabilities that
should be provided. This is provided through a Subscriber Identity
Module (SIM) @etsi-ts-131-102, a smart card @etsi-ts-102-221, that
stores encryption keys and application files used to verify the
subscriber in the telecommunications network. These SIM may have
varying support when roaming or if a device is 4G- or 5G-enabled.

The goal of this work is to develop a prototype for a set of tools
that include an SIM bank and a user equipment (UE) sink, and
inspire requirements for a mediation service.

First, the report recounts previous work, introduces the problem
and then provides relevant theory and background. Second, the
research methodology and materials used is introduced. Third is the
results showing prototype implementation and the conducted security
analysis. Fourth, an analysis based on the results with discussion
is given and last, conclusions are drawn.

== Background <sec:background>

Leissner Data AB (henceforth referred to as Leissner) is a Swedish
telecommunications firm from Trollhättan founded in 1969
@leissner-about. The company was not originally in
telecommunications but pivoted to an Internet Service Provider
(ISP) role in 1996 with the founding of the sister company
Götalandsnätet AB.

In 2001 Leissner started offering telecommunication systems and has
since extended to mobile platforms. These mobile platforms are, for
example, a complete virtual mobile core infrastructure such as
Packet Delivery Network Gateways (PGW), Short Message Service
Centers (SMSC) and Home Subscriber Servers (HSS)
@leissner-mobile-core.

After entering a partnership with Hi3G @leissner-about, Leissner
became a Mobile Virtual Network Enabler (MVNE) and sell SIM as part
of their product catalogue to Mobile Virtual Network Operators
(MVNO).

The aforementioned SIM are equipped with a digital profile
compatible with their systems. These digital profiles are preset
file structures and contents containing information such as the
International Mobile Subscriber Identifier (IMSI), Service Provider
Name (SPN) and encryption keys.

The digital profile may contain files that can be changed
@placeholder-source-for-smart-card-profiles, e.g. the SPN or the
Personal Identification Number (PIN) as well as files that may not
be changed, e.g. IMSI and Integrated Circuit Card Identifier
(ICCID). The design of the digital profiles is up to the agreement
between SIM supplier and the operator on determining which files
that should be writable or not.

Considering the aforementioned, a desire for a simplified test
platform arose. This test platform would allow hot-swapping of SIM
in a nearby location while having user equipment connecting from a
remote location, e.g. in another city or at a customer office. It
would be able to test reliability and function of the mobile core
platform with different SIM profiles. The functionality and
qualities of such a platform is described in
@sec:problem-formulation.

== Problem formulation <sec:problem-formulation>

#align(center)[
  #figure(
    image("images/system-simplified.mmd.png", width: 100%),
    caption: [
      Overview of the intended system diagram.
    ]
  ) <simplified-system-diagram>
]

This thesis answer some, but not all, of the below listed technical
problems and requests from Leissner. The full solution is a full
system with SIM banks, UE sinks and a mediator and is out of scope
of this report. An overview of the system is given by
@fig:simplified-system-diagram.

=== Requested functionality:
- *APDU forwarding:* This is for transporting APDU packets to and
  from an SIM to a UE, via an SIM bank and UE sink, optionally
  passing a mediator.
- *APDU tracing:* This is for logging information related to the
  transported APDU such as timing, data contents and SIM. This is
  out of scope.
- *APDU manipulation:* This is for changing information carried in
  the APDU. This is out of scope.
- *SIM reprogramming:* This generates APDU packets that are sent to
  an SIM that changes the information stored on the SIM. This is
  out of scope.

Only SIM are considered and no other smart card.

=== Requested qualities:
- *Scalability:* This is the ability to add and remove SIM, SIM
  banks and UE sinks from the system without configuration changes
  to the mediator. This process should be handled securely to
  disallow foreign actors to connect. This is out of scope.
- *Security:* APDU data may contain confidential information, such
  as encryption keys and identification numbers. When carried in a
  remote context, this information must be hidden. This will be
  analysed but not implemented. Safety risks are out of scope.
- *Commercial use:* Leissner, as a developer of mobile core
  products, has an interest in testing different SIM profiles in
  their core network. Though, the interest is not limited to only
  Leissner but extend to the business customers that operate their
  own mobile core networks.

== Previous work <sec:previous-work>

The main inspiration for this project is Osmocom's remSIM project
@osmocom-remsim. It is an open source implementation of an SIM bank
with APDU forwarding capabilities. Osmocom provides a user manual,
open hardware and source code that is used in this project.

Polygator @polygator-sim-bank and others offer SIM banks but the
products are too restrictive and incapable for Leissner's purposes.
The communications protocols when communicating with the SIM bank
may be proprietary and documentation unavailable.

== Theory <sec:theory>

This section elaborates on the necessary theory to understand the
use of SIM and how it relates to the implementation. 

=== Nomenclature <sec:nomenclature>

Definitions and descriptions of terminology used in the report are
provided here. A word is defined if it has a specific definition in
regards to this document or if the term carries extra relevance.

==== User Equipment <sec:user-equipment>

User Equipment (UE) is in regular telecommunication contexts what
may be used by a human, e.g. a mobile phone or wireless modems.
They may also be called mobile equipment or mobile stations.

==== Universal Subscriber Identity Module <sec:subscriber-identity-module>

A USIM is a smart card used for telecommunications @etsi-ts-131-102.
USIM was historically called SIM during 2G but SIM was superseded
by Universal SIM when 3G support was developed @etsi-ts-102-221.
A USIM enable 4G and 5G support given the appropriate digital
profile. It will be referred to SIM throughout this paper.

An SIM provides authentication capabilities for a single subscriber
by storing encryption keys and identification files within a
telecommunications application @etsi-ts-131-102.

==== Application Protocol Data Unit
<sec:application-protocol-data-unit>

Application Protocol Data Units (APDU) are the application-level
communications protocol residing on top of the transport layer of
a smart card and terminal interface @etsi-ts-102-221. These may be
file selections in the smart card, file reads, writes, or some other
operation supported by the card and terminal @etsi-ts-102-221.

==== SIM bank, SIM box or SIM farm <sec:sim-bank>

An SIM bank is a server of multiple SIM. It may be used by a
consuming service to either connect to multiple operators or
multiple configurations to a singular operator. One or more
consuming services may be connected @hyprms-sim-bank. An
illustrated SIM bank is given by @fig:sim-bank. 

#align(center)[
  #figure(
    image("images/simbank.mmd.png", width: 80%),
    caption: [
      An illustrative example of an SIM bank with six readers,
      connected to a network.
    ]
  ) <sim-bank>
]

The terms SIM bank, box, and farm all refer to the same thing and
may be used interchangeably. Different sources use the different
phrases but SIM bank will used in this paper.

==== UE sink <sec:ue-sink>

A UE sink is a device that is connected to a UE and may receive
APDU. These APDU may also be forwarded to the UE.

==== Mediator <sec:mediator>

The mediator is a middle-man service that manages connections
between SIM in the SIM bank and links them to a UE sink. It manages
Answer To Resets (ATR) for each SIM so UE can identify the card on
connection. The mediator also forwards, traces, and allows
manipulation of APDU.

=== Requirements engineering <sec:requirements-engineering>

This section cites Sommerville's seminal work Software Engineering
@sommerville-software-engineering. Requirements engineering is the
craft of finding, defining, validating, and changing requirements
for the solution of a problem. Requirements grow and change
alongside the lifecycle of a project and requirements engineering
is a formalization of how the process works. Requirements can be
separated into functional and non-functional where functional
requirements specify what a system should do while non-functional
requirements specify peripheral qualities of the system, such as
implementation constraints, response time and interface design.

+ *Elicitation* #linebreak() Elicitation includes gathering of
  requirements from stakeholders, establishing the kind,
  prioritizing and informally documenting said requirement.
  Stakeholders are people with legitimate interest in the system,
  accounting for members of the development team, customers,
  operational staff and others that are directly involved or
  affected by the system. 
+ *Specification* #linebreak() This entails formal or more
  technical specifications with details relevant to the
  implementers but not all stakeholders. Formalizations may take
  different appearances, as shown in figure 4.11 in
  @sommerville-software-engineering where natural language, both in
  unstructured and structured form, graphical notations and
  mathematical models are examples.
+ *Validation* #linebreak() This means testing the validity,
  consistency, completeness, realism and verifiability of
  requirements. The step is vital to ascertain correctness in the
  scope of the problem at hand and that a proposed solution
  fulfilling the requirements also solves the problem.
+ *Change* #linebreak() This specifically targets management of
  changes. Letting requirements change is important because new
  insights, knowledge, and changes in the surroundings spring up
  which affect the problem space. Key points in managing changes is
  to provide traceability, which entail the tracking of relevant
  stakeholders for a requirement, how the requirement has changed
  and why. The relationships between requirements need also be
  tracked to identify peripheral changes when a requirement is
  updated.

= Methodology <sec:methodology>

This chapter elaborates on the material, equipment, and methods
used to design a prototype for forwarding APDU packets.

== Materials and equipment <sec:materials>

#align(center)[
  #figure(
    image("images/prototype.mmd.png", width: 100%),
    caption: [
      Prototype system overview.
    ],
  ) <prototype-diagram>
]

This section lists the equipment, materials, and tools used in the
prototype. @fig:prototype-diagram shows how the of the physical
components are connected.

=== SIMtrace 2 <sec:materials:simtrace>

The SIMtrace 2 is designed by Osmocom and is an open hardware
platform with open source software @simtrace. It comes preloaded
with SIM tracing firmware called `trace` @simtrace-wiki but
additional firmware for smart card emulation exists, provided by
Osmocom. This firmware is called `cardem` @simtrace-wiki and is
used for communicating with user equipment, sending APDU packets
either constructed by software or forwarded from a remotely
accessed smart card. The hardware is capable of tracing and
emulating all kinds of ISO 7816 @etsi-ts-102-221 smart cards when
the T=0 protocol is used @simtrace-wiki but the focus is on SIM.

=== Raspberry Pi 4 <sec:materials:raspberry-pi>

Two Raspberry Pi 4 units are used because of their ubiquity. Any
modern computer running Linux is a viable choice but Raspberry Pi
are small, inexpensive and have a lower power drain.

=== Teltonika RUT200 v2.0 <sec:materials:teltonika>

This is used as user equipment the SIMtrace hardware running
`cardem` connects to and will therefore receive APDU packets. The
device provides 2G, 3G and 4G capabilities for mobile network
connectivity, the networks of which are authenticated to via the
remotely accessed smart card. It has a WiFi and Ethernet interface
allowing wired and wireless connections and internet access, acting
as a gateway between the mobile network and the local network.

=== Smart card readers, SIM, and router
<sec:materials:miscellaneous>

A Gemalto smart card reader is used where any PC/SC-compatible
would suffice. The smart card reader is connected to a Raspberry Pi
and provides access to a smart card. The APDU from the smart card
may be forwarded over IP to a destination device, in this case the
UE sink.

Leissner-provided SIM with a provisioned subscription are used
where any SIM would suffice. A basic router is used to provide IP
network access for the Raspberry Pi.

== Prototype implementation <sec:prototype-development>

The prototype @sommerville-software-engineering
@thomas-hunt-pragmatic-programmer is for exploring the technical
problems, and eliciting requirements to be used in future work.
Since only APDU forwarding is in scope, @fig:prototype-diagram is
the smallest design that tests the function. It entails the design
of a protocol that carries APDU information between the smart card
reader and the SIMtrace device.

The security considerations addressed by @sec:methodology:tara are
irrelevant in the prototype because it is set up in a laboratory
environment without external network access.

== Threat assessment and remediation analysis
<sec:methodology:tara>

The threat assessment and remediation analysis (TARA) @tara
responds to the security considerations in @sec:problem-formulation
by, with the help of STRIDE @stride, providing an analytical
framework to identify and classify security risks. The TARA is
conducted on the intended system overview shown in
@fig:simplified-system-diagram to elicit requirements.

= Results <sec:results>

The following sections provide the deliverables. The results may be
used for further work in authoring requirements and inspiring
future solutions, but they are not for production grade
applications.

== The prototype <sec:results:prototype>

This is implemented in two parts: the SIM bank and the UE sink, as
shown in @fig:prototype-diagram. The bank uses the PC/SC abstraction
for C++ provided by @electronic-id. The sink uses the SIMtrace 2 and
Osmocom core libraries @simtrace-library @osmocom-osmocore for
communication with and management of the SIMtrace 2 device and UE.
The implementation is available on GitHub @source.

The prototype has support for SIM bank to UE sink communication;
though, the UE sink has no ability to communicate with the UE
itself. The library and tools available from Osmocom were
nontrivial to integrate and mentioned @osmocom-limitations
limitations in pre-made firmware was incompatible with the use case
without alterations that were out of scope for this thesis.

The prototype is capable of carrying messages to and from the SIM
in the smart card reader (shown in @fig:prototype-diagram) over a
TCP connection. The TCP connection between the two Raspberry Pi
units carry a bespoke protocol designed called APDU and ATR
Tunneling Protocol, or ATP for short. The layout of the protocol is
shown in @fig:atp-diagram.

#align(center)[
  #figure(
    image("images/atp.mmd.png", width: 100%),
    caption: [
      APDU and ATR Tunneling Protocol (ATP).
    ],
  ) <atp-diagram>
]

The protocol first contains a version field for future-proofing
changes if it is employed in a production setting. Second, a field
for identifying what kind of message it is. Third, reserved bits
for aforementioned future-proofing. Last, a length field that
denotes the length the final data field may be.

The protocol design accounted for the possible size of APDU data
@apdu-data-size when the bit count for the length field was
determined. The other fields were adapted based on the remaining
bit count in a 32-bit integer while ensuring the type field had
space for the intended message types. The version field was chosen
arbitrarily but allows future modification of the protocol, such as
extending the type field into the reserved space and altering the
header completely. When every version change denotes a breaking
change, a total of 16 different versions were sufficient.

The protocol contains four message types: `APDU`, `ATR`,
`UnsetCard` and `Stop`. The first is used to denote that the data
payload is APDU data. The second is to denote ATR data payload. The
third is to inform the UE that the previously inserted card is
disconnected. The last type is used for halting the connection.

== Threat analysis <sec:result:tara>

= Analysis/Discussion <sec:analysis>

The prototype succeeded in what it set out to do; it forwards
APDU packets from an SIM to a UE. The UE, however, lacks in its
response due to a limitation in the firmware it has installed. This
firmware limitation is mentioned in @osmocom-limitations but
writing a patch for the problem is out of scope for this thesis.

The ATP protocol is designed with longevity in mind. This is
technically not a necessity for this purpose since the requirements
of long-term compliance with mixed versions is low when the service
is hosted by a singular company. It may serve a purpose if the
protocol design is openly available. Then, customers could
implement their own SIM banks, UE sinks or mediators and only buy
the component they wanted.

The prototype aimed for APDU forwarding as the core function
because it is the core of the system. APDU tracing, manipulation,
or SIM reprogramming serve no purpose if no APDU packets can be
transferred over the network. This leads into discussing potential
future work.

To remain in line with thorough software engineering practices
@sommerville-software-engineering, additional prototyping for the
aforementioned features may be conducted. Additionally, experiments
of the functionality in different programming languages can provide
insight on what languages are most optimal for the problem at hand.

The prototype in this report was implemented in C++ first because
that is one of the primary languages used at Leissner and second
because it could integrate with the PC/SC and Osmocom libraries.
Experiments in other languages should take library availability in
consideration, but also compare compiled languages like C++ and
Rust, with interpreted languages like JavaScript and Python. With
sensible measurements, this may provide insights on ergonomics,
performance bottlenecks, and different solution designs for the
main problem at hand.

#align(center)[
  #figure(
    image("images/process.mmd.png", width: 40%),
    caption: [
      The iterative development process. The dashed lines implies
      an optional order of execution
    ],
  ) <process-diagram>
]

Other future work is to base requirements specifications on this
paper and formalize a final design to be implemented. This should
take an iterative approach because of the nature of requirements
and how they may change over time. Akin to @fig:process-diagram.
The figure takes into mind that previously finished work, in this
report, may need alteration.

To keep in line with the documented nature of the approach so far,
a system implementation may adopt automated tooling such as
changelog generation @git-cliff, documentation generation from code
comments @doxygen, and architectural decision records (ADR) @adr. 

There were difficulties in getting `xmake` to build the Osmocom
libraries since they use their own build scripts instead of
commonly used Cmake or others. This led to requiring to install the
binaries with the system package manager and mark them as
system-only in `xmake`.

= Conclusions <sec:conclusions>

With plenty of additional work available, the prototype succeeded
in its goal of being able to forward APDU packets. With a protocol
designed for managing SIM and UE state future work has a baseline
to expand upon.
