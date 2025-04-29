#import "template.typ": thesis

#show: thesis.with(
  [A remote SIM bank, a UE sink, and a mediator -- Implementation
  of a utility for distributed coverage testing],
  [En fjärråtkomlig SIM-bank, en UE-uppsamlare och en medlare --
  Ett redskap för distribuerad täckningstestning],
  authors: (
    (
      name: "Hampus Avekvist",
      affiliation: "University West",
      email: "hampus.avekvist@hey.com",
    ),
  ),
  abstract: lorem(80),
  swedish_abstract: lorem(80),
  keywords: [
    5-10 words, separated by commas, describing the \
    contents of the report
  ],
  swedish_keywords: [
    Här skrivs 5-10 ord, åtskilda med kommatecken, som \
    beskriver rapportens innehåll, ej ord från titeln
  ],
  preface: [
    I'd like to thank Leissner Data AB for the opportunity to implement
    a novel technical system where much creativity had to be employed.
    I would also like to thank them for the independence and trust to
    work in my own way, allowing a thorough attempt at utilizing the
    skills taught during the computer engineering program. I'm giving a
    specific thanks to Peter Fässberg for the depths of knowledge he
    provided during the design and discussions of the implementation.

    TODO: Thank Hanna, Mikael and possibly others.

    An anecdote to illustrate what this report addresses:
    imagine having a mobile phone in China, and another in Spain. Due
    to the geographical, political and historical differences, it is
    not inconcievable that the mobile networks act differently. In a
    smaller scale, the same network operator in two different countries
    may support different functionality based on local laws and
    agreements with other operators. This means the same mobile device
    and SIM may have different behaviors based on location. This paper
    proposes (partly) a way to simplify location-dependent service
    testing.
  ],
  abbreviations: (
    (
      short: "3GPP",
      long: "3rd Generation Partnership Program",
    ),
    (
      short: "ADR",
      long: "Architectural Decision Record",
    ),
    (
      short: "APDU",
      long: "Application Protocol Data Unit",
    ),
    (
      short: "ATR",
      long: "Answer To Reset",
    ),
    (
      short: "ETSI",
      long: "European Telecommunications Standards Institute",
    ),
    (
      short: "ICCID",
      long: "Integrated Circuit Card Identifier",
    ),
    (
      short: "ISO",
      long: "International Organization for Standardization",
    ),
    (
      short: "ISP",
      long: "Internet Service Provider",
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
      short: "PC/SC",
      long: "Personal Computer / Smart Card",
    ),
    (
      short: "PIN",
      long: "Personal Identification Number",
    ),
    (
      short: "SIM",
      long: "Subscriber Identity Module",
    ),
    (
      short: "SPN",
      long: "Service Provider Name",
    ),
    (
      short: "TARA",
      long: "Threat assessment and remediation analysis",
    ),
    (
      short: "USIM",
      long: "Universal Subscriber Identity Module",
    ),
  ),
)

= Introduction <sec:introduction>

Connectivity is essential for modern society. The ability to keep
in touch, share knowledge and collaborate remotely is growing
evermore important. A fundamental group of technologies and
infrastructure enables these facets, all categorized within the
telecommunications umbrella. Telecommunication businesses around
the world compete to provide the best service, while assisting one
another to enable a greater reach.

For a user of these technologies and infrastructure, identification
is required to ascertain the authenticity and capabilities that
should be provided. This is provided through a Subscriber Identity
Module (SIM) @etsi-ts-131-102, a type of smart card
@etsi-ts-102-221, that stores encryption keys and application files
utilized to verify the subscriber in the telecommunications
network. These SIM may have varying support when roaming or if a
device is 4G- or 5G-enabled.

Considering the varying support for SIM configurations in different
telecommunication networks, there is an interest for utilities that
can easily enable testing, configuration and manipulation of SIM:s,
preferably with user equipment (UE) in arbitrary locations and a
bank of SIM in an easily (though not unauthorized) accessible
office space that allows hot-swapping of the SIM connected to the
differing UE.

The goal of this work is to develop said utilities that provide a
SIM bank, user equipment connectivity and a mediation service. The
mediation service enables pairing of SIM-to-UE with a logical
hot-swapping facility, rendering physical access to the SIM bank
only occasionally required rather than consistently.

The report starts off by introducing previous work in this sector,
formally introducing the problem and then provides necessary theory
and background information. The second chapter delves into the
methodology used in the design and implementation, alongside how
decisions were made and what material that was utilized. It also
shows the information collection process for relevant technical
specifications studied, used for the theory and background sections
in this chapter and the same information that guides the
implementation.

The third chapter addresses the results with a complete description
of the implementation alongside diagrams, measurement results from
the benchmarks and functionality tests described in
@sec:experimentation. In the fourth chapter, an analysis based on
the implementation is performed, addressing what could've been done
differently and how the tools, structure and method aided in the
solution for the problem at hand. Ethical considerations are
provided and in the fifth chapter conclusions are drawn.

== Background <sec:background>

Leissner Data AB (henceforth referred to as Leissner) is a Swedish
telecommunications firm from Trollhättan founded in 1969
@leissner-about. The company was not originally in
telecommunications but pivoted to an Internet Service Provider
(ISP) role in 1996 with the founding of the sister company
Götalandsnätet AB. In 2001 Leissner started offering
telecommunication systems and has since extended to mobile
platforms. These mobile platforms are, for example, complete
virtual mobile core infrastructure such as Packet Delivery Network
Gateways (PGW:s), Short Message Service Centers (SMSC:s) and Home
Subscriber Servers (HSS:s) @leissner-mobile-core.

After a partnership with Hi3G @leissner-about, Leissner became a
Mobile Virtual Network Enabler (MVNE) and in their product
catalogue, they sell SIM to the Mobile Virtual Network Operators
(MVNO). The aforementioned SIM:s is equipped with a digital profile
compatible with their systems. These digital profiles are preset
file structures and contents containing information such as the
International Mobile Subscriber Identifier (IMSI), Service Provider
Name (SPN) and encryption keys.

The digital profile may contain files that can be changed
@placeholder-source-for-smart-card-profiles, e.g. the SPN or the
Personal Identification Number (PIN) as well as files that may not
be changed, e.g. IMSI and Integrated Circuit Card Identifier
(ICCID). The design of the digital profiles is up to the agreement
between SIM supplier and the operator where which files should be
writeable or not can be decided.

Considering the aforementioned, a desire for a simplified test
platform arose. This test platform would allow hot-swapping of SIM
in a nearby location while having user equipment connecting from a
remote location, e.g. in another city or at a customer office to
test reliability and function of the mobile core platform with
different SIM profiles.

Functionality requested in such a platform include tracing of
application protocol data units (APDU:s) where the transmitted
APDU:s may be sniffed, or manipulation where transmitted APDU:s are
altered to e.g. transform one PIN into another or by changing an
SPN to something else. Reprogramming of SIM is another requested
capability which could replace existing software and offer all
SIM-related functionality inside a single system.

Lastly, support for other kinds of SIM than just physical in a
smart card reader is requested. This extends to virtual SIM in a
custom-developed runtime.

== Problem formulation <sec:problem-formulation>

The intention of this thesis is to implement a couple of technical
solutions addressing the requests from Leissner. The requests can
be dissolved into the following sections.

=== Functionality <sec:functionality>

Primary requested functionality is the ability to forward APDU:s
between an SIM and UE. This renders forwarding the initial problem
to solve and is dependent upon by secondary functionalities.
Tracing APDU:s and the ability to manipulate APDU:s extend the
forwarding functionality by either logging or changing the data.

Reprogrammability entails generating APDU:s from an alternative
source than from user equipment being sent directly to the SIM and
extends requirements to available functionality in a user
interface. This is out of scope for this project.

=== Scalability <sec:scalability>

Scalability in this case intends on providing an ability to add and
remove both SIM banks and UE from the system without configuration
changes in a central service. Both the SIM banks and the UE may be
in arbitrary locations.

=== Security considerations <sec:security-considerations>

The APDU:s may contain confidential information such as encryption
keys and identification numbers, normally not exposed due to the
locality of the SIM inside the UE but in the remote forwarding case
requiring hiding. Connectivity of the SIM bank and the UE to the
full system must also be handled in a secure way to not allow any
device to connect and get APDU delivered.

=== Commercialization <sec:commercialization>

The nature of being able to test mobile core products on different
SIM profiles is not limited to only the developers of the products
themselves but also the business customers buying and operating the
products. Therefore, there is an interest in having a sellable
service and easily deployed devices a customer can order.

=== Delimitations <sec:delimitations>

Manipulation of APDU:s, due to the added complexity in interface
design and time constraints will not be considered in this draft.
Similarly, the ability to reprogram SIM are out of scope for this
work as that relates to functionality not directly coupled with the
forwarding of APDU:s to and from an SIM and UE.

Only SIM will be considered in this implementation, excluding
general smart cards since the work is aimed at telecommunications.

While security considerations are accounted for, safety risks are
not included in this report.

== Previous work <sec:previous-work>

The primary inspiration and function description for this project
is Osmocom's remSIM project @osmocom-remsim. It is a complete open
source implementation of an SIM bank with forwarding capabilities to
user equipment. Osmocom provides a user manual, open hardware and
source code that is studied for this project's implementation.

There are additional solutions in the field of SIM banks with
similar capabilities to the Osmocom remSIM and the proposed
solution described in this document. These solutions, however, are
proprietary @polygator-sim-bank complete solutions, usually
offering a cloud service they own
@placeholder-source-cloud-sim-bank not enabling the capabilities
required for Leissner's purposes. They may also restrict the
communications protocol to the SIM bank to a proprietary,
undocumented protocol rendering implementation difficult.

The existing solutions miss out on some of the requested
functionality and while they could be extended upon, Leissner aims
to develop their own solution to have full control of their
offering. This is due to the proprietary nature of some of the
solutions, but also licensing issues with the Osmocom remSIM
solution, having a GPL-2.0 license while Leissner does not want to
distribute the source code for their products.

== Theory <sec:theory>

This section intends to elaborate on the necessary theory to
understand the utilization of SIM in the context of the
implementation and the related nomenclature.

=== Nomenclature <sec:nomenclature>

Definitions and descriptions of terminology commonly used in this
report are provided here. A word is explicitly stated if it has a
specific definition in regards to this document.

==== User Equipment <sec:user-equipment>

User Equipment (UE) is what in regular telecommunication contexts
may be used by a human, e.g. a mobile phone or wireless modems.
They may also be called mobile equipment or mobile stations.

==== Subscriber Identity Module <sec:subscriber-identity-module>

A SIM is a type of smart card utilized for the telecommunications
sector @etsi-ts-131-102. Historically it was called SIM during the
prime of 2G, but a later revision called Universal SIM (USIM) with
3G @etsi-ts-131-102 support was developed. USIM also enable 4G and
5G support given an appropriate digital profile. While USIM may be
the correct term, SIM will be used in its place, though
colloquially mean USIM.

A SIM is used to provide authentication capabilities by storing
encrypted keys and identification files within a telecommunications
application @etsi-ts-131-102. The SIM authenticates a specific
subscriber and their capabilities in a mobile network.

==== Application Protocol Data Unit
<sec:application-protocol-data-unit>

Application Protocol Data Units (APDU:s) are the application-level
communications protocol, residing on top of the transport layer of
a smart card and terminal interface @etsi-ts-102-221. These may be
file selections in the smart card, file reads, writes or some other
operation supported by the smart card @etsi-ts-102-221.

==== SIM bank, SIM box or SIM farm <sec:sim-bank>

A SIM bank is a device that provides connectivity to multiple SIM
simultaneously @hyprms-sim-bank. This may be used for connecting to
multiple operators or to have different SIM configurations from a
singular operator. One or more devices may connect to the bank and
choose which SIM to use, from which the devices can act as user
equipment.

The terms SIM bank, box and farm all refer to the same thing and
may be used interchangeably. Different sources use the different
phrases but SIM bank will used in this paper.

==== SIM consumer <sec:sim-consumer>

SIM consumer in relation to this work is the device that receives
forwarded APDU:s from a remote SIM bank. It is also this device
that is connected to user equipment and therefore forwards the
received APDU:s to the UE.

==== Mediator <sec:mediator>

The mediator is in this paper the service that manages connections
between SIM:s in the SIM bank and ties them to an SIM consumer. It
manages Answer To Resets (ATR:s) for each SIM so user equipment can
identify the type of card upon connection establishment. Alongside
ATR:s, it forwards APDU:s to support tracing and manipulation
capabilities.

=== Requirements engineering <sec:requirements-engineering>

This section cites from Sommerville's work Software Engineering
@sommerville-software-engineering. Requirements engineering is the
craft of finding, defining, validating and changing requirements
for the solution of a problem. Requirements grow and change
alongside the lifecycle of a project and requirements engineering
is a formalization of how the process works. Requirements can be
separated into functional and non-functional where functional
requirements specify what a system should do while non-functional
requirements specify peripheral qualities of the system, such as
implementation constraints, response time and interface design.

The first step is elicitation, which includes gathering of
requirements from stakeholders, establishing the kind of
requirement, prioritizing and informally documenting said
requirement. Stakeholders are people with legitimate interest in
the system in question, accounting for members of the development
team, customers, operational staff and others that are directly
involved or affected by the system. 

The second step is specification. This entails writing formal or
more technical specifications with additional details relevant to
the implementers but not all stakeholders. Formalizations may take
different appearances, as shown in figure 4.11 in
@sommerville-software-engineering where natural language, both in
unstructured and structured form, graphical notations and
mathematical models are examples of formalizations.

The third step is validation of the requirements. This means
testing the requirements for validity, consistency, completeness,
realism and verifiability. The step is vital to ascertain the
requirements are correct in the scope of the problem at hand and
that a proposed solution fulfilling the requirements also solves
the problem.

The last step is the change of requirements, specifically
management of changes. Letting requirements change is important
because new insights, knowledge and changes in the surroundings
spring up which affect the problem space. Key points in managing
changes is to provide traceability, which entails tracking of
relevant stakeholders for a requirement, how the requirement has
changed and why. The relationships between requirements need also
be tracked to identify peripheral changes when a requirement is
updated.

= Methodology <sec:methodology>

This chapter elaborates on the material, equipment and methods used
to derive a solution to the problem statements in
@sec:problem-formulation.

== Materials and equipment <sec:materials>

This section lists the equipment, material and tools used in the
implementation. @fig:simplified-system-diagram shows how the
majority of the physical components are connected.

#align(center)[
  #figure(
    image("images/system-simplified.mmd.png", width: 100%),
    caption: "Overview of the intended system diagram"
  ) <fig:simplified-system-diagram>
]

=== SIMtrace 2 <sec:materials:simtrace>

The SIMtrace 2 by Osmocom is an open hardware platform with open
source software @simtrace. It comes preloaded with SIM tracing
firmware called `trace` @simtrace-wiki but additional firmware for
smart card emulation exists, provided by Osmocom. This firmware is
called `cardem` @simtrace-wiki and is used for communicating with
user equipment, sending APDU:s either constructed by software or
forwarded from a remotely accessed smart card. The hardware is
capable of tracing and emulating all kinds of ISO 7816
@etsi-ts-102-221 smart cards when the T=0 protocol is used
@simtrace-wiki but the focus is on SIM.

=== Raspberry Pi 4 <sec:materials:raspberry-pi>

Two Raspberry Pi 4 units are used due to the ubiquity and
portability of the hardware platform. The main computer
requirements based on the proposed system are two Linux-based
devices with USB and IP network connectivity capabilities,
rendering most computer systems a viable choice where the choice of
Raspberry Pi is due to the small form factor, inexpensive hardware
and lower power drain, though none of those properties are a
requirement.

=== Teltonika RUT200 v2.0 <sec:materials:teltonika>

The Teltonika RUT200 v2.0 is used as provisional user equipment the
SIMtrace 2 hardware running `cardem` connects to, meaning the
receiver of forwarded APDU:s from a remotely accessed smart card.
The device provides 2G, 3G and 4G capabilities for mobile network
connectivity, authenticated to via the remotely accessed smart card
and has a WiFi and Ethernet interface allowing wired and wireless
connections and internet access, acting as a gateway between the
mobile network and the local network.

=== Smart card readers, SIM, server, and router
<sec:materials:miscellaneous>

Generic physical smart card readers are used for enabling access to
smart cards for the Raspberry Pi which will forward communications
from the smart card over IP to a destination device, e.g. a server
or a peer Raspberry Pi connected to an SIMtrace unit and user
equipment.

A server, virtual or not, is used as a central connectivity hub
enabling rerouting of SIM-to-UE communication to a either a
different UE or a different SIM. A basic router will be used to
provide IP network access for the Raspberry Pi and internet access
to the aforementioned connectivity hub.

=== Automated tooling <sec:materials:automated-tooling>

Tools for documentation generation will be utilized, both for
automatic generation of changelogs that document available
functionality in an operational perspective but also tools that
generate software library documentation from code comments.
Examples of such tools are Doxygen @doxygen for software library
documentation generation and git-cliff @git-cliff for changelog
generation.

To enable changelog generation, git-cliff requires a consistent
format of git commits @git-cliff called Conventional Commits
@conventional-commits. This, using git-cliff also enables automatic
version numbering @git-cliff-bump-version by following the Semantic
Versioning (SemVer) specification @semver. If required, additional
commit types will be created and documented with custom git parsers
added to git-cliff @git-cliff-tips-and-tricks.

Automated testing will be conducted to ensure that requirements are
upheld. The testing will be implemented using GoogleTest
@googletest and Google Mock @googletest. The tests will run on the
continuous integration/continuous deployment (CI/CD) platform
GitHub Actions @github-actions to ensure that no regressions have
been committed. Google Test will be used for authoring of tests and
Google Mock is utilized for mocks used by the tests.

Performance regressions will be detected by the use of automated
benchmarking. The primary implementation will be via Google's
microbenchmark support library @google-benchmark for smaller units
and `perf` @perf will be used for full application benchmarking if
necessary. `perf` will, however, not be automated.

== Prototype development <sec:prototype-development>

The initial development is for explorative purposes, to create a
prototype @sommerville-software-engineering
@thomas-hunt-pragmatic-programmer where ideas lead to requirements,
what to test for in the benchmarking and functionality experiment
listed in @sec:experimentation which alongside the TARA explained
in @sec:methodology:tara drives the decisions behind the final
implementation. Requirements engineering and experimentation,
elaborated on below, will use knowledge acquired in the prototyping
phase while also themselves provide additional information that can
sprout new concepts to initially implement in the prototype before
adding to the final implementation.

#align(center)[
  #figure(
    image("images/prototype.mmd.png", width: 90%),
    caption: "Prototype system overview",
  ) <fig:prototype-diagram>
]

The prototype, as shown in @fig:prototype-diagram, only requires
the functionality for communication between the two Raspberry Pi:s
used, while being restricted to only a singular SIM and singular
UE. The communication protocol between the two computers is a
simple insecure protocol on top of TCP instead of anything that may
be proposed in the TARA. The security considerations are not as
important in the lab environment the prototype is developed within
and has no customer-sensitive information, allowing an insecure
communications protocol.

== Threat assessment and remediation analysis
<sec:methodology:tara>

The threat assessment and remediation analysis (TARA) @tara
directly responds to the security considerations in
@sec:problem-formulation by, alongside STRIDE @stride, providing an
analytical framework to identify and classify security risks. The
TARA is conducted on the intended system overview shown in
@fig:simplified-system-diagram.

== Software requirements engineering
<sec:software-requirements-engineering>

Requirements elicitation will be used in tandem with the prototype
development and experimentation to narrow down and define what the
final product should look like. Specification of requirements will
use the collected knowledge and experience from prototyping and
experimentation both in structured natural language
@sommerville-software-engineering, as well as automated tests where
possible @test-cases-as-requirements. Validation will happen in
accordance with the following sections and change management will
be handled by version control management (using `git` @git) and
structured documents listing stakeholders, what and why, as well as
assigning a requirement identifier to each requirement. By using
`git`, each commit can contain relevant information on why a
requirement was changed.

=== Compilation of specifications
<sec:compilation-of-specifications>

Aside from the requirements for the software solution, the
specifications @etsi-ts-102-221, @etsi-ts-131-102 themselves pose
practical requirements for a functional system. Therefore, they are
an additional source of non-functional requirements. Following the
specifications also provide a validation angle for the authored
requirements.

=== Existing solutions and usage <sec:existing-solutions>

Existing solutions such as @osmocom-remsim provide knowledge to
compare with regarding how to solve the fundamental functional
requirements aiding in validation of the system requirements. The
existing material may also provide additional information not found
in the specifications, giving a fuller picture of system
requirements.

= Results <sec:results>

#align(center)[
  #figure(
    image("images/atp.mmd.png", width: 100%),
    caption: "APDU and ATR Tunneling Protocol (ATP)"
  ) <fig:atp-diagram>
]

#align(center)[
  #figure(
    image("images/system.mmd.png", width: 100%),
    caption: "High-level system diagram"
  ) <fig:system-diagram>
]

== Threat analysis <sec:result:tara>

== Deliverables <sec:results:deliverables>

=== SIM bank <sec:deliverables:sim-bank>

=== Consumer <sec:deliverables:consumer>

=== Mediator <sec:deliverables:mediator>

=== Documentation <sec:deliverables:documentation>

= Analysis/Discussion <sec:analysis>

== Ethical considerations <sec:ethical-considerations>

Idea: Criminal utilization, e.g. @alghawi-simbox.

= Conclusions <sec:conclusions>

== Future work <sec:future-work>

=== Experimentation <sec:experimentation>

In tandem with prototyping, experiments testing multiple
implementations of the PC/SC @pcsc library in different programming
languages have been conducted. The experiments are to guide
technical decisions for what language to use depending on which
implementation that fulfills requirements for communications with
smart cards. Four languages were chosen, JavaScript (on NodeJS) and
Python, to compare two higher-level languages, as well as C++ and
Rust to compare two lower-level languages.

The experiments test the performance and ability to communicate
with multiple smart cards at the same time, when transmitting
multiple APDU:s in quick succession.

Idea: Elaborate on what will be tested and what measurements will
be taken and why.

Note: This section is a stub and needs additional information.

=== Implementation <sec:implementation>

As already mentioned in chapter
@sec:software-requirements-engineering, tests will be used to
ensure requirements are being upheld. This leads swiftly into a
test-driven development approach @sommerville-software-engineering
while system design will be based around the non-functional
requirements. The process is iterative as mentioned in the previous
sections and shown in @fig:process-diagram, where new findings in
the prototyping or experimentation stages drive changes in the main
implementation. The implementation step will equally drive a need
to return to the prototype, experimentation and a change of
requirements.

#align(center)[
  #figure(
    image("images/process.mmd.png", width: 50%),
    caption: "The iterative development process. The dashed lines
	implies an optional order of execution",
  ) <fig:process-diagram>
]

Throughout development, designs decisions are documented in
architectural decision records (ADR:s) @adr @adr-github that
contain related requirements. Said ADR:s will be structured,
describing what ideas were considered, which one was chosen and why
to allow historical transparency in the system design.
