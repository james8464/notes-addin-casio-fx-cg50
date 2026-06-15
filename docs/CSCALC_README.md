# CSCALC.g3a README

## Purpose

`CSCALC.g3a` is the AQA A-level Computer Science 7517 calculation helper. It gives exam-style working for Paper 2 calculation tasks.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/CSCALC.g3a`

## Controls

- Same console shell as KhiCAS.
- Type a command at the prompt, e.g. `convert(45,10,16)`, then press `EXE`.
- Use the KhiCAS catalog/menu keys to insert commands; the CSCalc catalog contains only CS calculation commands.
- History, cursor movement, delete, redraw, and scrolling follow the original KhiCAS controls.

## Base / Number Representation

### Base Conversion

`convert(value,from_base,to_base)`

Aliases: `base`.

Examples:

- `convert(45,10,16)`
- `convert(101101,2,10)`
- `convert(2D,16,2)`
- `convert(71,8,10)`

### Short Conversion Helpers

`bin(value)`

`hex(value)`

`den(value,base)`

Examples:

- `bin(45)`
- `hex(45)`
- `den(2D,16)`

### Bit Width / Ranges

`bitsneeded(value)`

`bitsneeded(value,signed)`

`unsignedrange(bits)`

`twosrange(bits)`

`signmagrange(bits)`

`onesrange(bits)`

Examples:

- `bitsneeded(127)`
- `bitsneeded(-5,signed)`
- `unsignedrange(8)`
- `twosrange(8)`
- `signmagrange(8)`
- `onesrange(8)`

## Signed Binary

### Decode

`twosdec(bits)`

`signmagdec(bits)`

`onesdec(bits)`

Examples:

- `twosdec(11111011)`
- `signmagdec(10000101)`
- `onesdec(11111010)`

### Encode

`twos(value,bits)`

`signmag(value,bits)`

`ones(value,bits)`

Examples:

- `twos(-5,8)`
- `signmag(-5,8)`
- `ones(-5,8)`

### Arithmetic

`twosadd(bits1,bits2,width)`

`twossub(x,y,width)`

`binadd(bits1,bits2,width)`

`binsub(bits1,bits2,width)`

Examples:

- `twosadd(11111011,00000011,8)`
- `twossub(5,9,8)`
- `binadd(1011,0110,4)`
- `binsub(1011,0110,4)`

### Shifts

`shift(bits,direction,amount)`

`arithshift(bits,direction,amount)`

Examples:

- `shift(00101100,left,2)`
- `shift(00101100,right,2)`
- `arithshift(11101100,right,2)`

## Fixed Point

`fixed(bits)`

`fixedtc(bits)`

`fixedenc(value,left_bits,right_bits)`

`fixedfrac(numerator,denominator,right_bits)`

`fixedtcenc(value,left_bits,right_bits)`

Examples:

- `fixed(101.101)`
- `fixedtc(1101.10)`
- `fixedenc(5.625,3,3)`
- `fixedfrac(3,8,4)`
- `fixedtcenc(-2.5,4,3)`

## Floating Point

AQA style: mantissa and exponent are both two's complement; binary point is after the mantissa sign bit.

### Decode / Normalise

`floatdec(mantissa,exponent)`

`floatnorm(mantissa,exponent)`

`normal(mantissa)`

Examples:

- `floatdec(0101100,11101)`
- `floatnorm(00011010,0110)`
- `normal(0101100)`

### Encode / Range / Precision

`floatenc(value,mantissa_bits,exponent_bits)`

`floatrange(mantissa_bits,exponent_bits)`

`floatprecision(mantissa_bits,exponent)`

`floatnearest(value,mantissa_bits,exponent_bits)`

`floatcanrepresent(value,mantissa_bits,exponent_bits)`

Examples:

- `floatenc(12.75,8,4)`
- `floatrange(8,4)`
- `floatprecision(8,3)`
- `floatnearest(0.1,8,4)`
- `floatcanrepresent(500,8,4)`

### Floating Arithmetic

`floatadd(a,b,mantissa_bits,exponent_bits)`

`floatsub(a,b,mantissa_bits,exponent_bits)`

`floatmul(a,b,mantissa_bits,exponent_bits)`

`floatdiv(a,b,mantissa_bits,exponent_bits)`

Examples:

- `floatadd(1.5,2.25,8,4)`
- `floatsub(5,1.25,8,4)`
- `floatmul(1.5,2,8,4)`
- `floatdiv(3,2,8,4)`

## Error Detection / Bits

`parity(bits,even_or_odd)`

`repeatenc(bits,copies)`

`repeatdec(bits,copies)`

`xorbits(a,b)`

`andbits(a,b)`

`orbits(a,b)`

`notbits(bits)`

`grayenc(bits)`

`graydec(bits)`

`hamming(bits1,bits2)`

`hammingenc(data4bits)`

`checksum(value,width)`

Examples:

- `parity(1011001,even)`
- `repeatenc(101,3)`
- `repeatdec(111000111,3)`
- `xorbits(1010,1100)`
- `andbits(1010,1100)`
- `orbits(1010,1100)`
- `notbits(1010)`
- `grayenc(1011)`
- `graydec(1110)`
- `hamming(1010,1111)`
- `hammingenc(1011)`
- `checksum(173,8)`

## Storage / Transfer

`image(width,height,colour_depth)`

`imagecolors(width,height,colours)`

`colourdepth(colours)`

`colourcount(depth)`

`sound(sample_rate,duration,resolution,channels)`

`bitrate(bits,seconds)`

`bitratemb(megabytes,seconds)`

`bitratekb(kilobytes,seconds)`

`transfer(bits,rate)`

`transfermb(megabytes,mbit_per_second)`

`transferkb(kilobytes,kbit_per_second)`

Examples:

- `image(800,600,24)`
- `imagecolors(800,600,256)`
- `colourdepth(256)`
- `colourcount(8)`
- `sound(44100,60,16,2)`
- `bitrate(48000000,12)`
- `bitratemb(12,8)`
- `bitratekb(500,10)`
- `transfer(48000000,4000000)`
- `transfermb(12,8)`
- `transferkb(500,10)`

## Characters / Text

`ascii(code)`

`unicode(code)`

`chars(count,bits_per_char)`

`charset(symbols,bits_per_symbol)`

`symbolbits(symbol_count)`

Examples:

- `ascii(65)`
- `unicode(8364)`
- `chars(120,8)`
- `charset(96,7)`
- `symbolbits(130)`

## Compression

`compress(original,compressed)`

`dictcompress(original_bits,dictionary_bits,index_bits,count)`

`rle(runs,value_bits,count_bits)`

`rletext(text,value_bits,count_bits)`

`huffman(symbol1,freq1,bits1,...)`

Examples:

- `compress(1000,250)`
- `dictcompress(8000,300,5,700)`
- `rle(12,8,4)`
- `rletext(AAAABBCC,8,4)`
- `huffman(A,5,1,B,2,3,C,1,3)`

## Databases / Memory

`records(count,record_size)`

`addressspace(bits)`

`addressbits(locations)`

`memorycapacity(address_bits,word_bits)`

`hashmod(table_size,key1,key2,...)`

`hashlinear(table_size,key1,key2,...)`

`hashquadratic(table_size,key1,key2,...)`

Examples:

- `records(1200,32)`
- `addressspace(16)`
- `addressbits(65536)`
- `memorycapacity(16,8)`
- `hashmod(10,27,18,29)`
- `hashlinear(10,27,18,29)`
- `hashquadratic(10,27,18,29)`

## Algorithms / Tracing

`binarysearch(target,list...)`

`linearsearch(target,list...)`

`bubblesort(list...)`

`insertionsort(list...)`

`selectionsort(list...)`

`mergesort(list...)`

`stack(items...)`

`queue(items...)`

`preorder(nodes...)`

`inorder(nodes...)`

`postorder(nodes...)`

`dijkstra(start,target,node1,node2,weight,...)`

Examples:

- `binarysearch(7,1,3,5,7,9)`
- `linearsearch(7,4,7,1,9)`
- `bubblesort(5,1,4,2)`
- `insertionsort(5,1,4,2)`
- `selectionsort(5,1,4,2)`
- `mergesort(5,1,4,2)`
- `stack(3,4,pop,5)`
- `queue(3,4,pop,5)`
- `preorder(A,B,C,D,E)`
- `inorder(A,B,C,D,E)`
- `postorder(A,B,C,D,E)`
- `dijkstra(A,D,A,B,4,A,C,2,B,D,3,C,D,5)`

## Boolean / Logic

`bool_simplify(expression[, variables])`

Simplifies a Boolean expression and shows exam-style working.

Supported syntax:

- `and`, `or`, `not`
- `xor`, `nand`, `nor`, `implies`
- `*`, `.`, `+`
- apostrophe complement, e.g. `A'`
- brackets
- optional variable order, e.g. `A,B,C`

Method:

- normalises the expression into standard Boolean notation
- applies named laws where possible: identity, null/dominance, idempotent, complement, double complement, De Morgan, absorption, covering, distributive, bracket expansion, consensus, XOR, NAND, NOR and implication identities
- if laws stall before the shortest form, builds the truth table and performs exact K-map/minterm grouping
- prints the rule used beside each algebra line
- prints `Result:` as the final simplified form

Examples:
- `bool_simplify(A and not B)`
- `bool_simplify((A+B)*(A+C),A,B,C)`
- `bool_simplify(A*B+A*B'+A'*B)`
- `bool_simplify(A nand B)`
- `bool_simplify(A implies B)`
- `bool_simplify(not(not A and not B))`
- `bool_simplify((not A and not B) or (A and not B))`

`bool(expression)`

`truth(expression)`

`rpn(expression)`

`fsm(start,input,state,symbol,next,...)`

Examples:

- `bool(A and not B)`
- `truth(A and B)`
- `rpn(A B and not)`
- `fsm(A,010,A,0,B,A,1,A,B,0,A,B,1,B)`

## Command-Only Input

This app is command-only. It starts at an input prompt like KhiCAS: type the function and parameters, press `EXE`, then read the working lines.

## Full Command / Alias Index

Aliases call the same feature as their primary command.

- Base conversion: `bcd`, `bcdenc`, `bcddec`, `bcddecode`, `denbcd`, `bin`, `binary`, `tobin`, `hex`, `hexadecimal`, `tohex`, `den`, `denary`, `decimal`, `convert`, `base`
- Bit width/ranges: `bitsneeded`, `minbits`, `bitwidth`, `unsignedrange`, `urange`, `signmagrange`, `signmagnituderange`, `smrange`, `twosrange`, `tcrange`, `twoscomprange`, `onesrange`, `onescomprange`, `onescomplementrange`
- Signed binary: `signmag`, `signmagnitude`, `sm`, `signmagdec`, `signmagnitudedec`, `smdec`, `ones`, `onescomp`, `onescomplement`, `onesdec`, `onescompdec`, `onescomplementdec`, `twos`, `tc`, `twoscomp`, `twosdec`, `tcdec`, `twosdecode`, `twosadd`, `tcadd`, `twossub`, `tcsub`, `twossubtract`
- Binary arithmetic/bits: `binadd`, `binaryadd`, `addbits`, `binsub`, `binarysub`, `subtractbits`, `shift`, `binshift`, `arithshift`, `arithmeticshift`, `signedshift`, `parity`, `paritybit`, `repeatenc`, `repetitionenc`, `repeatencode`, `repeatdec`, `repetitiondec`, `majority`, `xorbits`, `andbits`, `orbits`, `notbits`, `invertbits`, `grayenc`, `binarytogray`, `graydec`, `graytobin`, `hamming`, `hammingdistance`, `bitdiff`, `hammingenc`, `hammingencode`, `hamming74`, `checksum`, `checksummod`, `binarychecksum`, `checkdigit`, `modcheck`, `weightedcheck`
- Fixed/floating point: `fixed`, `fixeddec`, `fixedtc`, `fixedtwos`, `fixedtwosdec`, `fixedenc`, `fixedencode`, `fixedpointenc`, `fixedfrac`, `fixedfraction`, `fixedfracenc`, `fixedtcenc`, `fixedtwosenc`, `fixedtwosencode`, `floatdec`, `fpdec`, `floatdecode`, `floatnorm`, `fpnorm`, `normalise`, `normalize`, `normal`, `floatenc`, `fpenc`, `floatencode`, `floatadd`, `fpadd`, `floatingadd`, `floatsub`, `fpsub`, `floatingsub`, `floatmul`, `fpmul`, `floatingmul`, `floatdiv`, `fpdiv`, `floatingdiv`, `floatprecision`, `fpprecision`, `floatstep`, `floatnearest`, `fpnearest`, `closestfloat`, `floatbitsadd`, `mantissabitsadd`, `exactfloatbits`, `floatrange`, `fprange`, `realrange`, `floatcanrepresent`, `floatrepresentable`, `fpcan`
- Storage/transfer: `image`, `bitmap`, `imagesize`, `imagecolors`, `bitmapcolors`, `colours`, `colourdepth`, `colordepth`, `bitsperpixel`, `colourcount`, `colorcount`, `pixelcolours`, `symbolbits`, `symbolsbits`, `bitsforsymbols`, `sound`, `audio`, `soundsize`, `bitrate`, `datarate`, `rate`, `bitratemb`, `datemb`, `mbyterate`, `bitratekb`, `datekb`, `kbyterate`, `transfer`, `transfertime`, `time`, `transfermb`, `megabytetransfer`, `mbtombit`, `transferkb`, `kilobytetransfer`, `kbtokbit`
- Text/compression: `ascii`, `charcode`, `codepoint`, `unicode`, `unicodepoint`, `ucode`, `chars`, `textsize`, `characters`, `charset`, `charsetsize`, `textsymbols`, `compress`, `compression`, `ratio`, `dictcompress`, `dictionary`, `lzdict`, `rle`, `runlength`, `runlengthencoding`, `rletext`, `rlestring`, `runencode`, `huffman`, `huff`, `huffmancode`
- Data/memory/hash: `sqlselect`, `selectwhere`, `sqlquery`, `sqlcount`, `countwhere`, `countrecords`, `records`, `recordsize`, `database`, `addressspace`, `addresses`, `addressbus`, `addressbits`, `minaddressbits`, `addresslines`, `memorycapacity`, `addresscapacity`, `memorybus`, `hashmod`, `hashtable`, `modhash`, `hashlinear`, `linearprobe`, `hashprobe`, `hashquadratic`, `quadraticprobe`, `quadprobe`
- Algorithms/tracing: `rpn`, `postfix`, `reversepolish`, `fsm`, `dfa`, `fsmout`, `mealy`, `dijkstra`, `shortestpath`, `shortpath`, `preordertree`, `treeprelinks`, `prelinks`, `inordertree`, `treeinlinks`, `inlinks`, `postordertree`, `treepostlinks`, `postlinks`, `preorder`, `treepre`, `pretraverse`, `inorder`, `treein`, `intraverse`, `postorder`, `treepost`, `posttraverse`, `stack`, `stacktrace`, `pushpop`, `queue`, `queuetrace`, `enqueue`, `binarysearch`, `binsearch`, `bsearch`, `linearsearch`, `linsearch`, `seqsearch`, `bubblesort`, `bubble`, `insertionsort`, `insertion`, `selectionsort`, `selection`, `mergesort`, `merge`
- Boolean/logic: `truth`, `truthtable`, `truthrows`, `bool`, `bool_simplify`, `boolsimplify`, `boolean`, `logic`, `posform`, `cnf`, `productofsums`, `truthbits`, `truthout`, `outputbits`, `maxterms`, `pos`, `zeros`, `minterms`, `kmap`, `karnaugh`, `kmapdc`, `mintermsdc`, `dcminterms`, `dontcare`, `nandform`, `onlynand`, `norform`, `onlynor`, `boolprove`, `provebool`, `logicprove`
