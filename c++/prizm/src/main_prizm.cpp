#include "device/device_solver.hpp"
#include "device/fixed_string.hpp"
#include "native_input.hpp"
#include "native_ui.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

namespace
{

constexpr int kMaxShellLines = 48;
constexpr int kEditorCapacity = 128;

template <int N>
constexpr int count_of(const char *const (&)[N])
{
    return N;
}

struct ShellLine {
    casio::device::FixedString<96> text;
};

struct CommandItem {
    const char *label;
    const char *insert;
};

struct FileItem {
    const char *label;
    int action;
};

enum FileAction
{
    FileToggleAngle = 1,
    FileClearInput,
    FileClearShell,
    FileInsertAns,
};

const CommandItem kAlgebraCommands[] = {
    {"simplify(", "simplify("},
    {"factor(", "factor("},
    {"partfrac(", "partfrac("},
    {"tcollect(", "tcollect("},
    {"texpand(", "texpand("},
    {"sum(", "sum("},
    {"oo", "oo"},
};

const CommandItem kAlgebraAllCommands[] = {
    {"simp", "simplify("},
    {"factor", "factor("},
    {"partfrac", "partfrac("},
    {"expand", "expand("},
    {"tcollect", "tcollect("},
    {"solve", "solve("},
    {"sum", "sum("},
    {"poly", "polynomial("},
    {"comp sq", "complete_square("},
    {"cmp", "compare("},
    {"match", "compare("},
    {"xform", "transform("},
    {"comp", "compose("},
    {"inv", "inverse("},
    {"rw", "rewrite("},
    {"dom/rng", "domain("},
    {"range", "range("},
};

const CommandItem kDeriveCommands[] = {
    {"n", "diff("},
    {"imp", "implicit_diff("},
    {"par", "param_diff("},
};

const CommandItem kIntegrateCommands[] = {
    {"int", "int("},
    {"limit", "limit("},
    {"binom", "binomial("},
    {"solve", "solve("},
    {"de", "de_solve("},
    {"defint", "defint("},
};

const CommandItem kCalculusCommands[] = {
    {"'", "'"},
    {"diff(", "diff("},
    {"second", "diff("},
    {"implicit", "implicit_diff("},
    {"param d", "param_diff("},
    {"integrate(", "integrate("},
    {"defint(", "defint("},
    {"limit(", "limit("},
    {"binomial(", "binomial("},
    {"solve(", "solve("},
};

const CommandItem kTrigCommands[] = {
    {"prove", "prove_trig("},
    {"xform", "transform_trig("},
    {"solve", "solve_trig("},
    {"rw", "rewrite_trig("},
    {"sin", "sin("},
    {"cos", "cos("},
    {"tan", "tan("},
    {"asin", "asin("},
    {"acos", "acos("},
};

const CommandItem kShellCommands[] = {
    {"sqrt", "sqrt("},
    {"abs", "abs("},
    {"log", "log("},
    {"ln", "ln("},
    {"exp", "exp("},
    {"pi", "pi"},
    {"suvat", "suvat("},
    {"ans", "ans"},
};

const CommandItem kStatsCommands[] = {
    {"binom", "binom("},
    {"binomcdf", "binomcdf("},
    {"normalcdf", "normalcdf("},
};

const CommandItem kArithCommands[] = {
    {"gcd", "gcd("},
    {"lcm", "lcm("},
    {"ifact", "factors("},
    {"divs", "divisors("},
    {"prime", "isprime("},
    {"fact!", "factorial("},
};

const FileItem kFileCommands[] = {
    {"toggle DEG/RAD", FileToggleAngle},
    {"clear input", FileClearInput},
    {"clear shell", FileClearShell},
    {"insert ans", FileInsertAns},
};

void add_shell_line(ShellLine *lines, int &count, const char *text)
{
    if(count >= kMaxShellLines) {
        for(int i = 1; i < kMaxShellLines; i++) {
            lines[i - 1].text.assign(lines[i].text.c_str());
        }
        count = kMaxShellLines - 1;
    }
    lines[count++].text.assign(text ? text : "");
}

void add_prompt_line(ShellLine *lines, int &count, const char *input)
{
    casio::device::FixedString<96> prompt;
    prompt.append(input ? input : "");
    prompt.append("=>");
    add_shell_line(lines, count, prompt.c_str());
}

void clamp_top(int count, int selected, int &top)
{
    if(count <= casio::prizm::kShellVisibleRows) {
        top = 0;
        return;
    }
    if(selected >= 0) {
        if(selected < top) top = selected;
        if(selected >= top + casio::prizm::kShellVisibleRows) {
            top = selected - (casio::prizm::kShellVisibleRows - 1);
        }
    }
    else {
        top = count - casio::prizm::kShellVisibleRows;
    }
    if(top < 0) top = 0;
    if(top > count - casio::prizm::kShellVisibleRows) top = count - casio::prizm::kShellVisibleRows;
}

void shell_mode(char *out, int capacity, bool degrees, bool uppercase)
{
    if(out == nullptr || capacity <= 0) return;
    casio::device::FixedString<16> mode;
    mode.append(degrees ? "DEG" : "RAD");
    if(uppercase) mode.append(" A");
    casio::device::copy_cstr(out, capacity, mode.c_str());
}

void render_shell(ShellLine *lines, int count, int top, int selected,
                  unsigned char *editor, int start, int cursor,
                  bool degrees, bool uppercase)
{
    const char *ptrs[kMaxShellLines];
    for(int i = 0; i < count; i++) ptrs[i] = lines[i].text.c_str();

    char mode[16];
    shell_mode(mode, (int)sizeof(mode), degrees, uppercase);
    casio::prizm::draw_shell("CasioCAS", mode, ptrs, count, top, selected, editor, start, cursor,
                             "algb", "calc", "trig", "cmds", uppercase ? "a<>A" : "A<>a", "File");
}

int select_menu(const char *title, const char *const *items, int count)
{
    int selected = 0;
    int top = 0;
    casio::prizm::draw_menu(title, items, count, selected, top);

    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT || key == KEY_CTRL_MENU) return -1;
        if(key == KEY_CTRL_EXE) return selected;
        if(key >= KEY_CHAR_1 && key <= KEY_CHAR_9) {
            int index = key - KEY_CHAR_1;
            if(index < count) return index;
        }
        if(key == KEY_CTRL_UP && selected > 0) selected--;
        if(key == KEY_CTRL_DOWN && selected + 1 < count) selected++;
        if(selected < top) top = selected;
        if(selected >= top + 5) top = selected - 4;
        casio::prizm::draw_menu(title, items, count, selected, top);
    }
}

template <int N>
int select_command(const char *title, const CommandItem (&commands)[N])
{
    const char *labels[N];
    for(int i = 0; i < N; i++) labels[i] = commands[i].label;
    return select_menu(title, labels, N);
}

int select_popup(const char *const *labels, int count, int fkey_index)
{
    if(labels == nullptr || count <= 0) return -1;
    int selected = 0;
    while(true) {
        casio::prizm::draw_fkey_popup(labels, count, selected, fkey_index);

        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU) return -1;
        if(key == KEY_CTRL_EXE) return selected;
        if(key >= KEY_CHAR_1 && key <= KEY_CHAR_9) {
            int index = key - KEY_CHAR_1;
            if(index < count) return index;
        }
        if(key == KEY_CTRL_UP && selected + 1 < count) selected++;
        if(key == KEY_CTRL_DOWN && selected > 0) selected--;
    }
}

template <int N>
int select_popup_command(const CommandItem (&commands)[N], int fkey_index)
{
    const char *labels[N];
    for(int i = 0; i < N; i++) labels[i] = commands[i].label;
    return select_popup(labels, N, fkey_index);
}

template <int N>
int select_file_command(const FileItem (&commands)[N])
{
    const char *labels[N];
    for(int i = 0; i < N; i++) labels[i] = commands[i].label;
    return select_menu("File", labels, N);
}

template <int N>
int select_popup_file(const FileItem (&commands)[N], int fkey_index)
{
    const char *labels[N];
    for(int i = 0; i < N; i++) labels[i] = commands[i].label;
    return select_popup(labels, N, fkey_index);
}

void insert_selected_line(ShellLine const *lines, int selected, unsigned char *editor, int &cursor)
{
    if(selected < 0) return;
    const char *line = lines[selected].text.c_str();
    const char *insert = line;
    if(casio::device::starts_with(line, "Answer: ")) insert = line + 8;

    casio::device::FixedString<96> clean;
    int n = casio::device::cstr_len(insert);
    int end = n;
    if(n >= 2 && insert[n - 2] == '=' && insert[n - 1] == '>') end = n - 2;
    for(int i = 0; i < end; i++) clean.append_char(insert[i]);
    casio::prizm::editor_insert_ascii(editor, kEditorCapacity, cursor, clean.c_str());
}

void run_shell_input(ShellLine *lines, int &count, const char *input)
{
    add_prompt_line(lines, count, input);

    casio::device::OutputLines out;
    bool ok = casio::device::solve(casio::device::Module::Shell, input, out);
    for(int i = 0; i < out.count(); i++) add_shell_line(lines, count, out.line(i));
    if(!ok && out.count() == 0) add_shell_line(lines, count, "Unsupported input.");
}

void insert_command(const CommandItem *items, int index, unsigned char *editor, int &cursor)
{
    if(items == nullptr || index < 0) return;
    casio::prizm::editor_insert_ascii(editor, kEditorCapacity, cursor, items[index].insert);
}

void open_commands(unsigned char *editor, int &cursor, int &start, bool &degrees,
                   ShellLine *lines, int &count)
{
    const char *groups[] = {
        "Algebra",
        "Calculus",
        "Trig",
        "Stats",
        "Arith",
        "Shell",
        "File",
    };
    int group = select_menu("Commands", groups, count_of(groups));
    if(group < 0) return;

    if(group == 0) {
        int c = select_command("Algebra", kAlgebraAllCommands);
        insert_command(kAlgebraAllCommands, c, editor, cursor);
    }
    else if(group == 1) {
        int c = select_command("Calculus", kCalculusCommands);
        insert_command(kCalculusCommands, c, editor, cursor);
    }
    else if(group == 2) {
        int c = select_command("Trig", kTrigCommands);
        insert_command(kTrigCommands, c, editor, cursor);
    }
    else if(group == 3) {
        int c = select_command("Stats", kStatsCommands);
        insert_command(kStatsCommands, c, editor, cursor);
    }
    else if(group == 4) {
        int c = select_command("Arith", kArithCommands);
        insert_command(kArithCommands, c, editor, cursor);
    }
    else if(group == 5) {
        int c = select_command("Shell", kShellCommands);
        insert_command(kShellCommands, c, editor, cursor);
    }
    else if(group == 6) {
        int c = select_file_command(kFileCommands);
        if(c < 0) return;
        int action = kFileCommands[c].action;
        if(action == FileToggleAngle) {
            degrees = !degrees;
            add_shell_line(lines, count, degrees ? "Angle mode: DEG." : "Angle mode: RAD.");
        }
        else if(action == FileClearInput) {
            casio::prizm::editor_from_ascii(editor, kEditorCapacity, "");
            start = 0;
            cursor = 0;
        }
        else if(action == FileClearShell) count = 0;
        else if(action == FileInsertAns) casio::prizm::editor_insert_ascii(editor, kEditorCapacity, cursor, "ans");
    }
}

}

extern "C" int main(void)
{
    Bdisp_EnableColor(1);

    ShellLine lines[kMaxShellLines];
    int line_count = 0;
    int top = 0;
    int selected = -1;
    bool degrees = true;
    bool uppercase = false;

    unsigned char editor[kEditorCapacity];
    casio::prizm::editor_from_ascii(editor, kEditorCapacity, "");
    int input_start = 0;
    int input_cursor = 0;

    while(true) {
        clamp_top(line_count, selected, top);
        render_shell(lines, line_count, top, selected, editor, input_start, input_cursor, degrees, uppercase);

        int key = 0;
        GetKey(&key);

        if(key == KEY_CTRL_EXIT || key == KEY_CTRL_MENU) break;
        if(key == KEY_CTRL_F1) {
            int c = select_popup_command(kAlgebraCommands, 0);
            insert_command(kAlgebraCommands, c, editor, input_cursor);
            selected = -1;
            continue;
        }
        if(key == KEY_CTRL_F2) {
            int c = select_popup_command(kCalculusCommands, 1);
            insert_command(kCalculusCommands, c, editor, input_cursor);
            selected = -1;
            continue;
        }
        if(key == KEY_CTRL_F3) {
            int c = select_popup_command(kTrigCommands, 2);
            insert_command(kTrigCommands, c, editor, input_cursor);
            selected = -1;
            continue;
        }
        if(key == KEY_CTRL_F4) {
            open_commands(editor, input_cursor, input_start, degrees, lines, line_count);
            selected = -1;
            continue;
        }
        if(key == KEY_CTRL_F5) {
            uppercase = !uppercase;
            selected = -1;
            continue;
        }
        if(key == KEY_CTRL_F6) {
            int c = select_popup_file(kFileCommands, 5);
            if(c >= 0) {
                int action = kFileCommands[c].action;
                if(action == FileToggleAngle) {
                    degrees = !degrees;
                    add_shell_line(lines, line_count, degrees ? "Angle mode: DEG." : "Angle mode: RAD.");
                }
                else if(action == FileClearInput) {
                    casio::prizm::editor_from_ascii(editor, kEditorCapacity, "");
                    input_start = 0;
                    input_cursor = 0;
                }
                else if(action == FileClearShell) line_count = 0;
                else if(action == FileInsertAns) casio::prizm::editor_insert_ascii(editor, kEditorCapacity, input_cursor, "ans");
            }
            selected = -1;
            continue;
        }

        if(key == KEY_CTRL_UP) {
            if(line_count > 0) {
                if(selected < 0) selected = line_count - 1;
                else if(selected > 0) selected--;
            }
            continue;
        }
        if(key == KEY_CTRL_DOWN) {
            if(selected >= 0 && selected + 1 < line_count) selected++;
            else selected = -1;
            continue;
        }

        if(key == KEY_CTRL_EXE) {
            if(selected >= 0) {
                insert_selected_line(lines, selected, editor, input_cursor);
                selected = -1;
                continue;
            }

            char input[128];
            casio::prizm::editor_to_ascii(editor, input, (int)sizeof(input), uppercase);
            if(input[0] != '\0') {
                run_shell_input(lines, line_count, input);
                casio::prizm::editor_from_ascii(editor, kEditorCapacity, "");
                input_start = 0;
                input_cursor = 0;
            }
            continue;
        }

        selected = -1;
        if(key == KEY_CTRL_AC) {
            casio::prizm::editor_from_ascii(editor, kEditorCapacity, "");
            input_start = 0;
            input_cursor = 0;
        }
        else if(key && key < 30000) {
            input_cursor = EditMBStringChar(editor, kEditorCapacity, input_cursor, key);
        }
        else {
            EditMBStringCtrl(editor, kEditorCapacity, &input_start, &input_cursor, &key, 2, 6);
        }
    }

    return 0;
}
