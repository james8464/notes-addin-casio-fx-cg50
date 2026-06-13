# CSCALC.g3a README

## Purpose

`CSCALC.g3a` is the AQA A-level Computer Science 7517 calculation helper. It gives exam-style working for Paper 2 calculation tasks.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/CSCALC.g3a`

## Controls

- `EXE`: insert selected command template.
- `F1`: help sheet.
- `EXIT`: back.

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

`bool(expression)`

`truth(expression)`

`rpn(expression)`

`fsm(start,input,state,symbol,next,...)`

Examples:

- `bool(A and not B)`
- `truth(A and B)`
- `rpn(A B and not)`
- `fsm(A,010,A,0,B,A,1,A,B,0,A,B,1,B)`

## Natural Text

Some short question-like inputs are recognised, but command syntax is the reliable path.

