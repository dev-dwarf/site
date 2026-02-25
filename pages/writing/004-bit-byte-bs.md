title: Bit Order, Byte Order, and other B.S.
date: Sat, 22 Feb 2026 10:25:00 MST
desc: It's called little-endian because the little bytes are at the end, right?
---
##intro Introduction

I often work with values packed into a small number of bytes using fixed-point; Generally this is used to pack as much information as possible into just a few bytes, either on embedded 
devices that use protocols like SPI or CAN bus, or when doing over-the-air communications through radios. Whatever the case may be, a common issue is that the low-level details of how multiple values are being packed into the same bytes are annoyingly under-specified. When digging into these details, I've had multiple disagreements about what encoding makes the most sense. 

Part of the problem is that there are many competing terms for the different ways of doing these encodings. For example, many articles refer to "left-most" or "right-most" bits. These terms only make sense in context with a correct accompanying visualization of the encoded data, making them ill-suited to general discussions of this topic. 

Another issue is that many standards that are supposed to describe these types of encodings fail to fully specify the information you need to decode the values. For example, when working with CAN bus, a common file format is @(https://www.csselectronics.com/pages/can-dbc-file-database-intro .dbc files), that specify how to decode fixed point values from a CAN message with a specific 11 bit ID and up to 8 bytes of payload. DBCs specify "signals", fixed point encoded values within the payload bytes. For the purpose of this article, only 3 fields are relevant, highlighted below:
```
  SG_ EngineSpeed : 24|16@1+ (0.125,0) [0|8031.875] "rpm" Vector__XXX
                    │  │  │ 
         start bit──┘  │  │
                       │  └───endianness (@1 == Little-Endian)
        bit length─────┘

```
So DBCs specify the start bits, bit length, and endianness. But this is not enough to decode 
values! There is no information about how start bits not divisible by 8 should work, which is
a very common case in these files. Tools that use .dbc files have you specify this either as 
"OSEK" which as far as I can tell always means `LSB0`, or as "Follows Endianness", which is
`MSB0` for Big-Endian and `LSB0` for Little-Endian. Some tools have you specify these 
properties on a per-signal basis; others assume that it is global to the entire file. If it's the latter when you needed the former, you will have to invert start bits manually to make the decoding make sense, which is tedious and easy to mess up.

When trying to choose a standard encoding for a codebase, I've had disagreements about what endianness made sense. Like in the case of .dbc files, this was because critical information was not being communicated: how to handle bit numbering within bytes. As someone who preferred little-endian, I had assumed any encoding would use `LSB0`. Similarly, my coworker who was advocating for big-endian assumed any encoding would use `MSB0`. We each thought the other was crazy until we got out the whiteboard and really dug into the details. In this article, I'll explain what I believe are the only 2 properties of these encodings that really matter, and dig into the different properties that the 4 resulting encoding options have.

---

##what What Really Matters

To cut through all the confusing conventions, terminology, and diagrams,
it's important to focus on the numeric value of the number being encoded;
`0x123` has a specific value that will be preserved by the compiler
regardless of machine endianness or human-level labels like "left-most" or "right-most".
There are only two choices that matter, and here is my best explanation of each:

1. **Byte Order**: if the numeric value of a byte increases with it's pointer address, the byte order is Little-Endian `(LE)`; if it decreases, then it's Big-Endian `(BE)`.

2. **Bit Order**: within a byte, if the bit number increases with the numeric value, then bit 0 is the Least Significant Bit `(LSB0)`; if it decreases, then bit 0 is the Most Significant Bit `(MSB0)`.

Taking every combination of these two gives us a table of four possiblities, with their properties summarized below:
!|left-border centered| || Little-Endian || Big-Endian |
!||LSB0|| monotonic bit-numbering, numerically consistent || <div class="centert"> numerically consistent</div> |
!||MSB0|| <div class="centert"> *???* </div> || monotonic bit-numbering, readable hex-dumps |
<br>

###mono Monotonic Bit-Numbering
For the `(LE, LSB0)` and `(BE, MSB0)` encodings, the bit number changes in the
same direction with the numeric value for both the bytes and bits. For both these variants, if 
you plot the bit number against the byte index + bit value within a byte, you get a 
straight line either increasing or decreasing respectively. For the other encodings, you 
instead get a zig-zag pattern that is trickier to think about when encoding data across byte 
boundaries:

<!--  
  Generated with:
  gnuplot bit-byte-bs-bit-index.gnu | sed -e 's/#FFFFFF/var(--bg)/g' -e 's/#ffffff/var(--bg)/g' -e 's/grey/var(--low)/g' -e 's/black/var(--text)/g' -e 's/Arial/Text/g' -e '/^$/d' > docs/assets/bit-byte-bs-bit-index.svg
-->
<svg 
 width="600" height="600"
 viewBox="0 0 600 600"
 xmlns="http://www.w3.org/2000/svg"
 xmlns:xlink="http://www.w3.org/1999/xlink"
>
<title>Gnuplot</title>
<desc>Produced by GNUPLOT 6.0 patchlevel 4 </desc>
<g id="gnuplot_canvas">
<rect x="0" y="0" width="600" height="600" fill="var(--bg)"/>
<defs>
  <circle id='gpDot' r='0.5' stroke-width='0.5' stroke='currentColor'/>
  <path id='gpPt0' stroke-width='0.222' stroke='currentColor' d='M-1,0 h2 M0,-1 v2'/>
  <path id='gpPt1' stroke-width='0.222' stroke='currentColor' d='M-1,-1 L1,1 M1,-1 L-1,1'/>
  <path id='gpPt2' stroke-width='0.222' stroke='currentColor' d='M-1,0 L1,0 M0,-1 L0,1 M-1,-1 L1,1 M-1,1 L1,-1'/>
  <rect id='gpPt3' stroke-width='0.222' stroke='currentColor' x='-1' y='-1' width='2' height='2'/>
  <rect id='gpPt4' stroke-width='0.222' stroke='currentColor' fill='currentColor' x='-1' y='-1' width='2' height='2'/>
  <circle id='gpPt5' stroke-width='0.222' stroke='currentColor' cx='0' cy='0' r='1'/>
  <use xlink:href='#gpPt5' id='gpPt6' fill='currentColor' stroke='none'/>
  <path id='gpPt7' stroke-width='0.222' stroke='currentColor' d='M0,-1.33 L-1.33,0.67 L1.33,0.67 z'/>
  <use xlink:href='#gpPt7' id='gpPt8' fill='currentColor' stroke='none'/>
  <use xlink:href='#gpPt7' id='gpPt9' stroke='currentColor' transform='rotate(180)'/>
  <use xlink:href='#gpPt9' id='gpPt10' fill='currentColor' stroke='none'/>
  <use xlink:href='#gpPt3' id='gpPt11' stroke='currentColor' transform='rotate(45)'/>
  <use xlink:href='#gpPt11' id='gpPt12' fill='currentColor' stroke='none'/>
  <path id='gpPt13' stroke-width='0.222' stroke='currentColor' d='M0,1.330 L1.265,0.411 L0.782,-1.067 L-0.782,-1.076 L-1.265,0.411 z'/>
  <use xlink:href='#gpPt13' id='gpPt14' fill='currentColor' stroke='none'/>
  <filter id='textbox' filterUnits='objectBoundingBox' x='0' y='0' height='1' width='1'>
    <feFlood flood-color='var(--bg)' flood-opacity='1' result='bgnd'/>
    <feComposite in='SourceGraphic' in2='bgnd' operator='atop'/>
  </filter>
  <filter id='var(--low)box' filterUnits='objectBoundingBox' x='0' y='0' height='1' width='1'>
    <feFlood flood-color='lightvar(--low)' flood-opacity='1' result='var(--low)'/>
    <feComposite in='SourceGraphic' in2='var(--low)' operator='atop'/>
  </filter>
</defs>
<g fill="none" color="var(--bg)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,510.00 L73.14,510.00  '/> <g transform="translate(55.75,513.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" >-1</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,481.06 L73.14,481.06  '/> <g transform="translate(55.75,484.96)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,452.12 L73.14,452.12  '/> <g transform="translate(55.75,456.02)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 1</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,423.18 L73.14,423.18  '/> <g transform="translate(55.75,427.08)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 2</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,394.24 L73.14,394.24  '/> <g transform="translate(55.75,398.14)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 3</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,365.30 L73.14,365.30  '/> <g transform="translate(55.75,369.20)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 4</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,336.36 L73.14,336.36  '/> <g transform="translate(55.75,340.26)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 5</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,307.42 L73.14,307.42  '/> <g transform="translate(55.75,311.32)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 6</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,278.48 L73.14,278.48  '/> <g transform="translate(55.75,282.38)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 7</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,249.53 L73.14,249.53  '/> <g transform="translate(55.75,253.43)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 8</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,220.59 L73.14,220.59  '/> <g transform="translate(55.75,224.49)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 9</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,191.65 L73.14,191.65  '/> <g transform="translate(55.75,195.55)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 10</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,162.71 L73.14,162.71  '/> <g transform="translate(55.75,166.61)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 11</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,133.77 L73.14,133.77  '/> <g transform="translate(55.75,137.67)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 12</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,104.83 L73.14,104.83  '/> <g transform="translate(55.75,108.73)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 13</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,75.89 L73.14,75.89  '/> <g transform="translate(55.75,79.79)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 14</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,46.95 L73.14,46.95  '/> <g transform="translate(55.75,50.85)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 15</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,18.01 L73.14,18.01  '/> <g transform="translate(55.75,21.91)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" > 16</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,510.00 L64.14,501.00  '/> <g transform="translate(64.14,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" >-1</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M94.18,510.00 L94.18,501.00  '/> <g transform="translate(94.18,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M124.22,510.00 L124.22,501.00  '/> <g transform="translate(124.22,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 1</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M154.26,510.00 L154.26,501.00  '/> <g transform="translate(154.26,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 2</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M184.30,510.00 L184.30,501.00  '/> <g transform="translate(184.30,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 3</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M214.34,510.00 L214.34,501.00  '/> <g transform="translate(214.34,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 4</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M244.38,510.00 L244.38,501.00  '/> <g transform="translate(244.38,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 5</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M274.42,510.00 L274.42,501.00  '/> <g transform="translate(274.42,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 6</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M304.46,510.00 L304.46,501.00  '/> <g transform="translate(304.46,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 7</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M334.50,510.00 L334.50,501.00  '/> <g transform="translate(334.50,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 8</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M364.54,510.00 L364.54,501.00  '/> <g transform="translate(364.54,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 9</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M394.58,510.00 L394.58,501.00  '/> <g transform="translate(394.58,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 10</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M424.62,510.00 L424.62,501.00  '/> <g transform="translate(424.62,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 11</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M454.66,510.00 L454.66,501.00  '/> <g transform="translate(454.66,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 12</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M484.70,510.00 L484.70,501.00  '/> <g transform="translate(484.70,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 13</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M514.74,510.00 L514.74,501.00  '/> <g transform="translate(514.74,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 14</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M544.78,510.00 L544.78,501.00  '/> <g transform="translate(544.78,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 15</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M574.82,510.00 L574.82,501.00  '/> <g transform="translate(574.82,531.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" > 16</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,18.01 L64.14,510.00 L574.82,510.00 M574.82,18.01 M64.14,18.01  '/></g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='rgb(192, 192, 192)' stroke-dasharray='2.5,4.0'  d='M334.51,509.99 L334.51,18.00  '/></g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
  <g id="gnuplot_plot_1" ><title>LE, LSB0</title>
<g fill="none" color="var(--bg)" stroke="var(--text)" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <g transform="translate(133.68,585.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" >LE, LSB0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <use xlink:href='#gpPt6' transform='translate(94.18,481.06) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(124.22,452.12) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(154.26,423.18) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(184.30,394.24) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(214.34,365.30) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(244.38,336.36) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(274.42,307.42) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(304.46,278.48) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(334.50,249.53) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(364.54,220.59) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(394.58,191.65) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(424.62,162.71) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(454.66,133.77) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(484.70,104.83) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(514.74,75.89) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(544.78,46.95) scale(2.70)' color='rgb(  0, 102, 204)'/>
  <use xlink:href='#gpPt6' transform='translate(163.35,582.00) scale(2.70)' color='rgb(  0, 102, 204)'/>
</g>
  </g>
  <g id="gnuplot_plot_2" ><title>BE, MSB0</title>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <g transform="translate(260.14,585.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" >BE, MSB0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <use xlink:href='#gpPt6' transform='translate(94.18,46.95) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(124.22,75.89) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(154.26,104.83) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(184.30,133.77) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(214.34,162.71) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(244.38,191.65) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(274.42,220.59) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(304.46,249.53) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(334.50,278.48) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(364.54,307.42) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(394.58,336.36) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(424.62,365.30) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(454.66,394.24) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(484.70,423.18) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(514.74,452.12) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(544.78,481.06) scale(2.70)' color='rgb( 34, 139,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(289.81,582.00) scale(2.70)' color='rgb( 34, 139,  34)'/>
</g>
  </g>
  <g id="gnuplot_plot_3" ><title>LE, MSB0</title>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <g transform="translate(386.60,585.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" >LE, MSB0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <use xlink:href='#gpPt6' transform='translate(94.18,278.48) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(124.22,307.42) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(154.26,336.36) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(184.30,365.30) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(214.34,394.24) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(244.38,423.18) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(274.42,452.12) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(304.46,481.06) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(334.50,46.95) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(364.54,75.89) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(394.58,104.83) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(424.62,133.77) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(454.66,162.71) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(484.70,191.65) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(514.74,220.59) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(544.78,249.53) scale(2.70)' color='rgb(230, 126,  34)'/>
  <use xlink:href='#gpPt6' transform='translate(416.27,582.00) scale(2.70)' color='rgb(230, 126,  34)'/>
</g>
  </g>
  <g id="gnuplot_plot_4" ><title>BE, LSB0</title>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <g transform="translate(513.06,585.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="end">
    <text><tspan font-family="Text" >BE, LSB0</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="3.00" stroke-linecap="butt" stroke-linejoin="miter">
  <use xlink:href='#gpPt6' transform='translate(94.18,249.53) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(124.22,220.59) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(154.26,191.65) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(184.30,162.71) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(214.34,133.77) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(244.38,104.83) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(274.42,75.89) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(304.46,46.95) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(334.50,481.06) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(364.54,452.12) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(394.58,423.18) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(424.62,394.24) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(454.66,365.30) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(484.70,336.36) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(514.74,307.42) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(544.78,278.48) scale(2.70)' color='rgb(142,  68, 173)'/>
  <use xlink:href='#gpPt6' transform='translate(542.73,582.00) scale(2.70)' color='rgb(142,  68, 173)'/>
</g>
  </g>
<g fill="none" color="var(--bg)" stroke="rgb(142,  68, 173)" stroke-width="2.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="2.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="var(--text)" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <path stroke='var(--text)'  d='M64.14,18.01 L64.14,510.00 L574.82,510.00 M574.82,18.01 M64.14,18.01  '/>  <g transform="translate(19.18,264.01) rotate(-90.00)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" >bit number</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
  <g transform="translate(319.48,558.90)" stroke="none" fill="var(--text)" font-family="Text" font-size="12.00"  text-anchor="middle">
    <text><tspan font-family="Text" >8\*(byte index) + bit value</tspan></text>
  </g>
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
<g fill="none" color="var(--text)" stroke="currentColor" stroke-width="1.00" stroke-linecap="butt" stroke-linejoin="miter">
</g>
</g>
</svg>

###cons Numerical Consistency
The encodings that use `LSB0` are numerically consistent if their endianness 
matches platform endianness, in that the bit numbers align with their numeric 
values. For example, when using a numerically consistent encoding, serializing 
`0x100` with a start bit of 0 will result in only bit 8 being set.

In contrast `MSB0` encodings will use numerically non-contiguous bits when
encoding across byte boundaries. It's hard to say this is a disadvantage in practice;
@(#BE_MSB0-1 in the example code for reading bits) this adds a very small amount
of complexity to the code, which doesnt really matter after you implement that function
in your codebase and hopefully never have to think about it again.

###hex Readable Hex Dumps
The `(BE, MSB0)` approach has an advantage that the others don't, which is that 
if you do a hex dump of the encoded data, values that have been encoded will be
visible in the same form as their associated numeric value:
```hex-dump
read start=4, len=16:
  hex dump: EF CD AB 
(BE, MSB0): 0xFCDA
(BE, LSB0): 0xECDB
(LE, LSB0): 0xBCDE
(LE, MSB0): 0xACDF
```
This property has arbitrarily assigned to `(BE, MSB0)` by the following coincidence:
we read numbers @(https://www.youtube.com/watch?v=Wu1kSXpVV8Y&t=1743s right-to-left), but
print hex dumps with the bytes going left to right, like english text. These two ways of writing the numbers mirror each other, and using the `(BE, MSB0)` encoding mirrors them again
(as shown in the plot above). These two mirror-ings cancel out, giving us the same sequence
in both representations.

Personally, I don't assign much value to this property. Unless the values are nibble-aligned and the data is mostly zero like in the above example, hex-dumps of each of the encoding types all become equally readable; they are all hard to read. This would be a bit better with binary dumps, but then the information density is much lower.

###only4 Only 4 Encodings??
I'm assuming above that when serializing values, the LSB and MSB are written to
the LSB and MSB of the storage region; this excludes "bit-reversed" 
representations, like the below example of encoding the 8-bit value `0xF5`, starting at bit 4:

<svg xmlns="http://www.w3.org/2000/svg" width="600" height="300" viewBox="0 0 920 420">
  <g transform="translate(60, 60)">
    <text x="380" y="-25" font-family="Code" font-size="24" fill="var(--text)">Normal</text>
    <rect x="220" y="10" width="360" height="100" fill="#e3f2fd" opacity="0.45"/>
    <rect x="220" y="10" width="400" height="100" fill="var(--high)"/>
    <g font-family="Code" font-size="26" text-anchor="middle" fill="var(--text)">
      <rect x= "20" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x= "45" y="65">0</text>
      <rect x= "70" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x= "95" y="65">0</text>
      <rect x="120" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="145" y="65">0</text>
      <rect x="170" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="195" y="65">0</text>
      <rect x="220" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="245" y="65">1</text>
      <rect x="270" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="295" y="65">1</text>
      <rect x="320" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="345" y="65">1</text>
      <rect x="370" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="395" y="65">1</text>
      <rect x="420" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="445" y="65">0</text>
      <rect x="470" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="495" y="65">1</text> 
      <rect x="520" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="545" y="65">0</text>
      <rect x="570" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="595" y="65">1</text>
      <rect x="620" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="645" y="65">0</text>
      <rect x="670" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="695" y="65">0</text>
      <rect x="720" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="745" y="65">0</text>
      <rect x="770" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="795" y="65">0</text>
    </g>
    <g font-family="Code" font-size="1rem" fill="var(--text)" text-anchor="middle">
      <text x="45"  y="145">15</text>
      <text x="95"  y="145">14</text>
      <text x="145" y="145">13</text>
      <text x="195" y="145">12</text>
      <text x="245" y="145">11</text><text x="240" y="30">MSB</text>
      <text x="295" y="145">10</text>
      <text x="345" y="145">9</text>
      <text x="395" y="145">8</text>
      <text x="445" y="145">7</text>
      <text x="495" y="145">6</text>
      <text x="545" y="145">5</text>
      <text x="595" y="145">4</text><text x="600" y="30">LSB</text>
      <text x="645" y="145">3</text>
      <text x="695" y="145">2</text>
      <text x="745" y="145">1</text>
      <text x="795" y="145">0</text>
    </g>
  </g>
  <g transform="translate(60, 240)">
    <rect x="220" y="10" width="400" height="100" fill="var(--high)"/>
    <g font-family="Code" font-size="26" text-anchor="middle" fill="var(--text)">
      <rect x= "20" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x= "45" y="65">0</text>
      <rect x= "70" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x= "95" y="65">0</text>
      <rect x="120" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="145" y="65">0</text>
      <rect x="170" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="195" y="65">0</text>
      <rect x="220" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="245" y="65">1</text>
      <rect x="270" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="295" y="65">0</text>
      <rect x="320" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="345" y="65">1</text>
      <rect x="370" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="395" y="65">0</text>
      <rect x="420" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="445" y="65">1</text>
      <rect x="470" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="495" y="65">1</text> 
      <rect x="520" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="545" y="65">1</text>
      <rect x="570" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="595" y="65">1</text>
      <rect x="620" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="645" y="65">0</text>
      <rect x="670" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="695" y="65">0</text>
      <rect x="720" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="745" y="65">0</text>
      <rect x="770" y="10" width="50" height="100" fill="none" stroke="var(--text)" />
      <text x="795" y="65">0</text>
    </g>
    <g font-family="Code" font-size="1rem" fill="var(--text)" text-anchor="middle">
      <text x="45"  y="145">15</text>
      <text x="95"  y="145">14</text>
      <text x="145" y="145">13</text>
      <text x="195" y="145">12</text>
      <text x="245" y="145">11</text><text x="240" y="30">LSB</text>
      <text x="295" y="145">10</text>
      <text x="345" y="145">9</text>
      <text x="395" y="145">8</text>
      <text x="445" y="145">7</text>
      <text x="495" y="145">6</text>
      <text x="545" y="145">5</text>
      <text x="595" y="145">4</text><text x="600" y="30">MSB</text>
      <text x="645" y="145">3</text>
      <text x="695" y="145">2</text>
      <text x="745" y="145">1</text>
      <text x="795" y="145">0</text>
    </g>
    <text x="380" y="180" font-family="Code" font-size="24" fill="var(--text)">Reversed</text>
  </g>
</svg>

The only place where I've seen bit-reversal is with misconfigured serial ports
where a device is sending data MSB first but the host machine is reading it as 
LSB first. I imagine that any similar wire format could have similar issues, 
but it seems best to just configure the host machine appropriately. Considering 
bit-reversal would add `(LE, MSB0, reversed)` and `(BE, LSB0, reversed)` 
options that have more of the desireable properties above, but would also add 
the non-sensical `(LE, LSB0, reversed)` and `(BE, MSB0, reversed)`. 
To keep this article focused, I am not considering these additional options.

---
##code Example Code
To demonstrate, below is some example code to read bits in each of the 4 
encoding variants. Each function makes assumptions for clarity, that should be 
checked or guaranteed in production. I have elided these checks because when 
I've written usage code with these functions those properties have always been 
guaranteed by the time I'm calling the function. Here are the problems that can 
result:
- Undefined behavior on shifts if `len > 64`.
- Buffer overflow if `(start + len + 7)/8 >= sizeof(buf)`.
Each of the function decodes in 3 parts:
1. Partial bits in start byte. Early exit for small values.
2. Full bytes.
3. Partial bits in last byte.
Each of the functions puts the bits directly in their final location; This is not the only way 
to "assemble" the bits, but seemed to make the differences between the 4 versions the 
clearest. All of the functions should work regardless of platform endianness; everything is 
handled byte-by-byte, and the bit operations are based on numeric values so will set the 
correct bits regardless.

<div class="centered w50">
```COMMON
// Common defines
#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
```
</div>
<div class="flex">
```LE_LSB0
u64 read_bits_le_lsb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  u8 n = 0;

  u8 ir = start & 0x7;
  if (ir) {
    n = 8 - ir;
    v = buf[i++] >> ir;
    if (len < n) {
      return v & ((1<<len)-1);
    }
  }

  while (n+8 < len) {
    v |= buf[i++] << n;
    n += 8;
  }

  u8 r = len - n;
  if (r) {
    v |= (buf[i++] & ((1<<r)-1)) << n;
  }

  return v;
}
```
```LE_MSB0
u64 read_bits_le_msb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  u8 n = 0;

  u8 ir = start & 0x7;
  if (ir) {
    n = 8-ir;
    v = buf[i++] & ((1<<n)-1);
    if (len <= n) {
      return v >> (n-len); 
    }
  }

  while (n+8 <= len) {
    v |= buf[i++] << n;
    n += 8;
  }

  u8 r = len - n;
  if (r) {
    v |= (buf[i] >> (8-r)) << n;
  }

  return v;
}
```
```BE_LSB0
u64 read_bits_be_lsb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  
  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8 - ir;
    v = buf[i++] >> ir;
    if (len > n) {
      len -= n;
      v = v << len;
    } else {
      return v & ((1<<len)-1);
    }
  }

  while (len >= 8) {
    len -= 8;
    v |= buf[i++] << len;
  }

  if (len) {
    v |= buf[i] & ((1<<len)-1);
  }

  return v;
}
```
```BE_MSB0
u64 read_bits_be_msb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  
  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8-ir;
    v = buf[i++] & ((1<<n)-1);
    if (len > n) {
      len -= n;
      v = v << len;
    } else {
      return v >> (n-len); 
    }
  }

  while (len >= 8) {
    len -= 8;
    v |= buf[i++] << len;
  }

  if (len) {
    v |= buf[i] >> (8-len);
  }

  return v;
}
```
</div>

All of these implementations are about equally complex. The approach taken is byte-by-byte, 
which I would expect to be faster than going bit-by-bit, but other than that this code has not 
been profiled or optimized for speed at all. I have tested each function against a variety
of start/len inputs with different bit patterns. My code for doing that is not polished,
but you can find it 
@(https://github.com/dev-dwarf/site/blob/main/pages/writing/bit-byte-bs.c on github). These 
functions are a very generic solution; for simple
use-cases you could write the masks and shifts manually, or if you are generating code to 
decode a fixed structure like a .dbc you could generate the specific masks and shifts for each
signal and avoid the need for any conditionals.

--- 
## My Favorite Option

My preference whenever you have the choice of what encoding to use is `(LE, LSB0)`. Little-Endian is the most common endianness on modern machines. This means we get the consistency benefit of having our encoding endianness match platform endianness if we choose `LSB0`. The resulting bit-numbering is monotonic so the range of bits to encode any given value will always occupy a numerically-contiguous region. Before writing this article I thought that for these two reasons that it was an obvious choice. I feel less strongly after writing the article; ?(except for LE MSB0, each approach) has shown advantages and disadvantages in understanding, and the code for reading values had a similar level of complexity for each variant. But ultimately to me `(LE, LSB0)` still makes the most sense. 

To encourage its use, here is an additional code sample for writing bits with `(LE, LSB0)`,
to go with the `read_bits_le_lsb0` sample above. The structure is very similar, although writing takes extra care to avoid overwriting adjacent bit packed values.
```WRITE_LE_LSB0
void write_bits_le_lsb0(u8 *buf, u32 start, u8 len, u64 value) {
  u8 i = start >> 3;

  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8 - ir;
    if (n < len) {
      buf[i] = (buf[i] & ((1<<ir)-1)) | (((u8)value) << ir);
      i++;
      len -= n;
      value >>= n;
    } else {
      u8 mask = ((1<<len)-1) << ir;
      buf[i] = (buf[i] & ~mask) | ((value << ir) & mask);
      return;
    }
  }

  while (len >= 8) {
    buf[i++] = ((u8)value);
    len -= 8;
    value >>= 8;
  }

  if (len > 0) {
    u8 mask = ((1<<len)-1);
    buf[i] = (buf[i] & ~mask) | (value & mask);
  }
}
```
