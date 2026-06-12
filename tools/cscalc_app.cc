#include "casio_suite_ui.hpp"
#include "cscalc_engine.hpp"

static const char *const menu_items[] = {
  "Free input",
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

static const char *const baseconv[] = {
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
  "Addition:",
  "0+0=0, 0+1=1, 1+1=10.",
  "1+1+1=11.",
  "Logical left shift multiplies by 2.",
  "Logical right shift divides by 2.",
  "Arithmetic right shift keeps sign bit.",
  "Discard overflow beyond fixed width."
};

static const char *const fixedp[] = {
  "Fixed point:",
  "binary point has fixed position.",
  "Left side weights: 2^0,2^1,...",
  "Right side: 2^-1,2^-2,...",
  "Add weights for 1 bits.",
  "For negative, decode two's complement first.",
  "Precision = value of lowest bit."
};

static const char *const floatdec[] = {
  "AQA floating point:",
  "mantissa and exponent use two's comp.",
  "Decode exponent as signed integer e.",
  "Mantissa binary point after sign bit.",
  "Value = mantissa * 2^e.",
  "Move binary point by e places.",
  "Then convert fixed point to denary."
};

static const char *const floatenc[] = {
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
  "Normalised AQA form:",
  "positive mantissa begins 01.",
  "negative mantissa begins 10.",
  "If not, shift mantissa left/right.",
  "Adjust exponent opposite direction.",
  "More mantissa bits = more precision.",
  "More exponent bits = more range."
};

static const char *const storage[] = {
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
  "binarysearch(target,list...)",
  "linearsearch(target,list...)",
  "bubblesort(list...)",
  "insertionsort(list...)",
  "Binary search needs sorted data.",
  "Outputs each comparison/pass."
};

static void run_input(unsigned *tick) {
  char input[96] = "floatdec(0101100,11101)";
  if (!ui_input("CSCalc input", input, sizeof(input), tick)) return;
  char lines[CSCALC_MAX_LINES][CSCALC_LINE_LEN];
  int count = cscalc_eval(input, lines);
  const char *ptrs[CSCALC_MAX_LINES];
  for (int i = 0; i < count; ++i) ptrs[i] = lines[i];
  ui_wait_page("Working", ptrs, count, tick);
}

static void open_item(int sel, unsigned *tick) {
  if (sel == 0) run_input(tick);
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

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
  int sel = 0, top = 0, count = sizeof(menu_items)/sizeof(menu_items[0]);
  bool rv = ui_r_visible(tick);
  ui_menu("AQA CS Calc", menu_items, count, top, sel, rv);
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(tick);
    if (nr != rv) { rv = nr; ui_menu("AQA CS Calc", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU) return 0;
    if (key == KEY_CTRL_UP && sel > 0) { --sel; if (sel < top) --top; ui_menu("AQA CS Calc", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_DOWN && sel + 1 < count) { ++sel; if (sel >= top + 7) ++top; ui_menu("AQA CS Calc", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_EXE || key == KEY_CTRL_F1) { open_item(sel, &tick); rv = ui_r_visible(tick); ui_menu("AQA CS Calc", menu_items, count, top, sel, rv); }
    OS_InnerWait_ms(35);
  }
}
