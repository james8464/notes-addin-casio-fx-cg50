#include "casio_suite_ui.hpp"
#include "cscalc_engine.hpp"

static const char *const menu_items[] = {
  "Command input",
  "Base conversions",
  "Two's complement",
  "Binary arithmetic",
  "Fixed point",
  "Floating decode",
  "Floating encode",
  "Normalisation",
  "Range/precision",
  "Image storage",
  "Sound storage",
  "Bit rate",
  "Compression",
  "Char storage",
  "Databases",
  "Algorithms"
};

static const char *const command_templates[] = {
  "floatdec(0101100,11101)",
  "convert(45,10,16)",
  "twos(-5,8)",
  "binadd(1011,0110,4)",
  "fixedenc(5.625,3,3)",
  "floatdec(0101100,11101)",
  "floatenc(12.75,8,4)",
  "floatnorm(00011010,0110)",
  "floatrange(8,4)",
  "image(800,600,24)",
  "sound(44100,60,16,2)",
  "bitrate(48000000,12)",
  "compress(1000,250)",
  "chars(120,8)",
  "records(1200,32)",
  "binarysearch(7,1,3,5,7,9)"
};

static const char *const baseconv[] = {
  "EXE inserts: convert(45,10,16)",
  "AQA 7517 Paper 2 data rep.",
  "Binary to denary:",
  "sum bit * 2^position.",
  "Denary to binary:",
  "repeated divide by 2.",
  "Binary to hex:",
  "group bits in 4s from right.",
  "Hex to binary:",
  "replace each hex digit by 4 bits."
};

static const char *const twos[] = {
  "EXE inserts: twos(-5,8)",
  "n-bit range:",
  "-2^(n-1) to 2^(n-1)-1.",
  "Decode if MSB=0: unsigned value.",
  "Decode if MSB=1:",
  "invert bits, add 1, make negative.",
  "Encode negative:",
  "write positive magnitude, invert, add 1.",
  "Subtraction: add two's complement.",
  "Overflow if sign result is impossible."
};

static const char *const binaryarith[] = {
  "EXE inserts: binadd(1011,0110,4)",
  "Addition:",
  "0+0=0, 0+1=1, 1+1=10.",
  "1+1+1=11.",
  "Logical left shift multiplies by 2.",
  "Logical right shift divides by 2.",
  "Arithmetic right shift keeps sign bit.",
  "Discard overflow beyond fixed width."
};

static const char *const fixedp[] = {
  "EXE inserts: fixedenc(5.625,3,3)",
  "Fixed point:",
  "binary point has fixed position.",
  "Left side weights: 2^0,2^1,...",
  "Right side: 2^-1,2^-2,...",
  "Add weights for 1 bits.",
  "For negative, decode two's complement first.",
  "Precision = value of lowest bit."
};

static const char *const floatdec[] = {
  "EXE inserts: floatdec(0101100,11101)",
  "AQA floating point:",
  "mantissa and exponent use two's comp.",
  "Decode exponent as signed integer e.",
  "Mantissa binary point after sign bit.",
  "Value = mantissa * 2^e.",
  "Move binary point by e places.",
  "Then convert fixed point to denary."
};

static const char *const floatenc[] = {
  "EXE inserts: floatenc(12.75,8,4)",
  "Encode decimal:",
  "convert magnitude to binary.",
  "Move point to normalised mantissa.",
  "Normal positive starts 01...",
  "Normal negative starts 10...",
  "Exponent is point movement.",
  "Store exponent in two's complement.",
  "Round mantissa to available bits."
};

static const char *const normalise[] = {
  "EXE inserts: floatnorm(00011010,0110)",
  "Normalised AQA form:",
  "positive mantissa begins 01.",
  "negative mantissa begins 10.",
  "If not, shift mantissa left/right.",
  "Adjust exponent opposite direction.",
  "More mantissa bits = more precision.",
  "More exponent bits = more range."
};

static const char *const storage[] = {
  "EXE inserts: image(800,600,24)",
  "Image size:",
  "width * height * colour_depth bits.",
  "Sound size:",
  "sample_rate * duration * resolution bits.",
  "Stereo doubles sound size.",
  "Bit rate:",
  "bits transferred / seconds.",
  "Convert: 8 bits = 1 byte.",
  "Use powers of 1000 unless stated."
};

static const char *const compress[] = {
  "EXE inserts: compress(1000,250)",
  "Compression ratio:",
  "original size / compressed size.",
  "Saving:",
  "(old-new)/old * 100%.",
  "RLE stores value and run length.",
  "Dictionary stores repeated patterns once.",
  "Lossy removes data.",
  "Lossless can reconstruct original."
};

static const char *const chars[] = {
  "EXE inserts: chars(120,8)",
  "Character storage:",
  "bits = characters * bits per char.",
  "ASCII commonly 7 or 8 bits.",
  "Unicode uses more bits per char.",
  "ascii(65): code to char/binary/hex.",
  "ASCII code for A: preserves case.",
  "More bits allow more symbols:",
  "2^bits possible codes.",
  "Include metadata only if stated."
};

static const char *const db[] = {
  "EXE inserts: records(1200,32)",
  "Database calculation checks:",
  "Records * fields * field size.",
  "Primary key uniquely identifies record.",
  "Foreign key references another table.",
  "Entity integrity: primary key not null.",
  "Referential integrity: valid foreign key.",
  "Normalisation reduces duplication."
};

static const char *const algos[] = {
  "Trace commands:",
  "EXE inserts: binarysearch(7,1,3,5,7,9)",
  "binarysearch(target,list...)",
  "linearsearch(target,list...)",
  "bubblesort(list...)",
  "insertionsort(list...)",
  "selectionsort/mergesort",
  "stack(3,4,pop,5)",
  "queue(3,4,pop,5)",
  "preorder(a,b,c,d,e)",
  "inorder/postorder use same nodes.",
  "dijkstra(a,d,a,b,4,...)",
  "Binary search needs sorted data.",
  "Outputs each comparison/pass."
};

static const char *const base_ex[] = {
  "convert(45,10,16)",
  "convert(101101,2,10)",
  "convert(2D,16,2)",
  "bitsneeded(-5,signed)",
  "unsignedrange(8)"
};

static const char *const signed_ex[] = {
  "twos(-5,8)",
  "twosdec(11111011)",
  "twosrange(8)",
  "twosadd(11111011,00000011,8)",
  "twossub(5,9,8)"
};

static const char *const binary_ex[] = {
  "binadd(1011,0110,4)",
  "binsub(1011,0110,4)",
  "shift(00101100,left,2)",
  "shift(00101100,right,2)",
  "arithshift(11101100,right,2)"
};

static const char *const fixed_ex[] = {
  "fixed(101.101)",
  "fixedtc(1101.10)",
  "fixedenc(5.625,3,3)",
  "fixedfrac(3,8,4)",
  "fixedtcenc(-2.5,4,3)"
};

static const char *const float_ex[] = {
  "floatdec(0101100,11101)",
  "floatenc(12.75,8,4)",
  "floatnorm(00011010,0110)",
  "floatrange(8,4)",
  "floatnearest(0.1,8,4)"
};

static const char *const storage_ex[] = {
  "image(800,600,24)",
  "sound(44100,60,16,2)",
  "bitrate(48000000,12)",
  "imagesize(1024,768,8)",
  "soundsize(8000,30,8)"
};

static const char *const misc_ex[] = {
  "compress(1000,250)",
  "chars(120,8)",
  "records(1200,32)",
  "binarysearch(7,1,3,5,7,9)",
  "bubblesort(5,1,4,2)"
};

static void copy_initial(char *dst, int dst_len, const char *src) {
  int i = 0;
  if (!src) src = "";
  for (; i + 1 < dst_len && src[i]; ++i) dst[i] = src[i];
  dst[i] = 0;
}

static void run_input(const char *initial, unsigned *tick) {
  char input[96];
  copy_initial(input, sizeof(input), initial);
  if (!ui_input("CSCalc input", input, sizeof(input), tick)) return;
  char lines[CSCALC_MAX_LINES][CSCALC_LINE_LEN];
  int count = cscalc_eval(input, lines);
  const char *ptrs[CSCALC_MAX_LINES];
  for (int i = 0; i < count; ++i) ptrs[i] = lines[i];
  ui_wait_page("Working", ptrs, count, tick);
}

static void open_item(int sel, bool help, unsigned *tick) {
  if (!help) {
    run_input(command_templates[sel], tick);
    return;
  }
  if (sel == 0) ui_wait_page("Command input", baseconv, sizeof(baseconv)/sizeof(baseconv[0]), tick);
  else if (sel == 1) ui_wait_page("Base conversions", baseconv, sizeof(baseconv)/sizeof(baseconv[0]), tick);
  else if (sel == 2) ui_wait_page("Two's complement", twos, sizeof(twos)/sizeof(twos[0]), tick);
  else if (sel == 3) ui_wait_page("Binary arithmetic", binaryarith, sizeof(binaryarith)/sizeof(binaryarith[0]), tick);
  else if (sel == 4) ui_wait_page("Fixed point", fixedp, sizeof(fixedp)/sizeof(fixedp[0]), tick);
  else if (sel == 5) ui_wait_page("Floating decode", floatdec, sizeof(floatdec)/sizeof(floatdec[0]), tick);
  else if (sel == 6) ui_wait_page("Floating encode", floatenc, sizeof(floatenc)/sizeof(floatenc[0]), tick);
  else if (sel == 7 || sel == 8) ui_wait_page(menu_items[sel], normalise, sizeof(normalise)/sizeof(normalise[0]), tick);
  else if (sel >= 9 && sel <= 11) ui_wait_page(menu_items[sel], storage, sizeof(storage)/sizeof(storage[0]), tick);
  else if (sel == 12) ui_wait_page("Compression", compress, sizeof(compress)/sizeof(compress[0]), tick);
  else if (sel == 13) ui_wait_page("Char storage", chars, sizeof(chars)/sizeof(chars[0]), tick);
  else if (sel == 14) ui_wait_page("Databases", db, sizeof(db)/sizeof(db[0]), tick);
  else ui_wait_page("Algorithms", algos, sizeof(algos)/sizeof(algos[0]), tick);
}

static void open_examples(int sel, unsigned *tick) {
  if (sel == 0 || sel == 1) ui_wait_page("Base examples", base_ex, sizeof(base_ex)/sizeof(base_ex[0]), tick);
  else if (sel == 2) ui_wait_page("Signed examples", signed_ex, sizeof(signed_ex)/sizeof(signed_ex[0]), tick);
  else if (sel == 3) ui_wait_page("Binary examples", binary_ex, sizeof(binary_ex)/sizeof(binary_ex[0]), tick);
  else if (sel == 4) ui_wait_page("Fixed examples", fixed_ex, sizeof(fixed_ex)/sizeof(fixed_ex[0]), tick);
  else if (sel >= 5 && sel <= 8) ui_wait_page("Float examples", float_ex, sizeof(float_ex)/sizeof(float_ex[0]), tick);
  else if (sel >= 9 && sel <= 11) ui_wait_page("Storage examples", storage_ex, sizeof(storage_ex)/sizeof(storage_ex[0]), tick);
  else ui_wait_page("More examples", misc_ex, sizeof(misc_ex)/sizeof(misc_ex[0]), tick);
}

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
  int sel = 0, top = 0, count = sizeof(menu_items)/sizeof(menu_items[0]);
  bool rv = ui_r_visible(tick);
  ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK");
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(tick);
    if (nr != rv) { rv = nr; ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK"); }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU || key == KEY_CTRL_F6) return 0;
    if (ui_menu_handle_key(key, count, 7, &sel, &top)) ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK");
    if (key == KEY_CTRL_EXE || key == KEY_CTRL_F2) { open_item(sel, false, &tick); rv = ui_r_visible(tick); ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK"); }
    if (key == KEY_CTRL_F1) { open_item(sel, true, &tick); rv = ui_r_visible(tick); ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK"); }
    if (key == KEY_CTRL_F3) { open_examples(sel, &tick); rv = ui_r_visible(tick); ui_menu_keys("AQA CS Calc", menu_items, count, top, sel, rv, "HELP", "RUN", "EXS", "", "", "BACK"); }
    OS_InnerWait_ms(35);
  }
}
