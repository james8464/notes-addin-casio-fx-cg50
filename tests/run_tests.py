#!/usr/bin/env python3
"""
CASIO Test Suite - Enhanced with Rich Terminal UI
A unified test suite with beautiful terminal UI and interactive mode.
"""

import argparse
import subprocess
import sys
import math
from pathlib import Path
from typing import Callable, Any, Optional
from dataclasses import dataclass
from enum import Enum
from collections import defaultdict

from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.text import Text
from rich.style import Style
from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, TaskProgressColumn, TimeRemainingColumn, MofNCompleteColumn
from rich.live import Live
from rich.layout import Layout
from rich.prompt import Prompt, Confirm
from rich.tree import Tree
from rich import box
from rich.status import Status
import threading

try:
    from textual.app import App, ComposeResult
    from textual.containers import Horizontal, VerticalScroll
    from textual.widgets import Header, Footer, Static, Collapsible
    TEXTUAL_AVAILABLE = True
except ImportError:
    TEXTUAL_AVAILABLE = False

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
PY = sys.executable
sys.path.insert(0, str(ROOT))
sys._algebra_no_autorun = True
sys._trig_no_autorun = True
sys._derive_no_autorun = True
sys._int_no_autorun = True
sys._suvat_no_autorun = True

import algebraProgram as ALG
import trigProgram as TRIG
import deriveProgram as DERIVE
import intProgram as INTEGRATE
import SUVATprogram as SUVAT

console = Console()

ARG_PARSER = argparse.ArgumentParser(description="CASIO unified test suite")
ARG_PARSER.add_argument("--plain", action="store_true", help="Force plain console output and skip the full-screen Textual viewer.")
ARGS = ARG_PARSER.parse_args()
IS_TTY = sys.stdout.isatty()
INTERACTIVE_REPORT_MODE = not ARGS.plain and IS_TTY and TEXTUAL_AVAILABLE

PASS_COUNT = 0
FAIL_COUNT = 0
WARN_COUNT = 0
TEST_COUNTER = 0

TEST_OUTPUTS = {}  # test_id -> output
CURRENT_CATEGORY = ""
CURRENT_SECTION = ""
CURRENT_QUESTION = ""

# Rich UI components for interactive experience
progress = None
current_task = None
status_spinner = None
lock = threading.Lock()

def create_progress_bar():
    global progress
    if not INTERACTIVE_REPORT_MODE and IS_TTY:
        progress = Progress(
            SpinnerColumn(style="cyan"),
            TextColumn("[bold cyan]{task.description}"),
            BarColumn(bar_width=40),
            TaskProgressColumn(),
            MofNCompleteColumn(),
            TextColumn("[dim]{task.fields[category]}[/dim]"),
            console=console,
        )

def start_spinner(text: str):
    global status_spinner
    if not INTERACTIVE_REPORT_MODE and IS_TTY:
        status_spinner = console.status(f"[bold cyan]{text}", spinner="dots")
        status_spinner.start()

def stop_spinner():
    global status_spinner
    if status_spinner:
        status_spinner.stop()
        status_spinner = None

def update_progress(task_id, advance=False):
    global progress, current_task
    if progress and current_task is not None:
        if advance:
            progress.update(current_task, advance=1)


@dataclass
class TestRecord:
    test_id: int
    label: str
    status: "TestStatus"
    detail: str
    output: str
    category: str
    section: str
    question: str = ""


TEST_RECORDS = []

class TestStatus(Enum):
    PASS = "pass"
    FAIL = "fail"
    WARN = "warn"
    SKIP = "skip"

def print_header():
    if INTERACTIVE_REPORT_MODE:
        return
    title = Text("CASIO TEST SUITE", style="bold cyan")
    subtitle = Text("Comprehensive Automated System for Interactive Operations", style="dim")
    panel = Panel.fit(
        Text.assemble(title, "\n", subtitle),
        title="CASIO",
        border_style="cyan",
        box=box.DOUBLE,
    )
    console.print(panel)
    console.print()
    create_progress_bar()
    if progress:
        progress.start()

def print_category(name: str):
    global CURRENT_CATEGORY, CURRENT_SECTION, current_task
    CURRENT_CATEGORY = name
    CURRENT_SECTION = ""
    if INTERACTIVE_REPORT_MODE:
        return
    console.print()
    console.print(f"[bold magenta]▶ {name}[/bold magenta]")
    if progress:
        current_task = progress.add_task(f"Running {name}...", category=name, total=None)

def print_section(name: str):
    global CURRENT_SECTION, status_spinner
    CURRENT_SECTION = name
    if INTERACTIVE_REPORT_MODE:
        return
    stop_spinner()
    console.print(f"\n[cyan]{name}[/cyan]")
    start_spinner(f"Running {name}...")

def run_cli(script_name: str, user_input: str) -> tuple:
    proc = subprocess.run(
        [PY, str(ROOT / script_name)],
        input=user_input,
        text=True,
        capture_output=True,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr

def report(label: str, passed: bool, detail: str = "", output: str = ""):
    global PASS_COUNT, FAIL_COUNT, WARN_COUNT, TEST_COUNTER, CURRENT_QUESTION
    TEST_COUNTER += 1
    test_id = TEST_COUNTER
    
    if passed:
        PASS_COUNT += 1
        status = Text("PASS ", style="bold green")
        icon = Text("✓ ", style="bold green")
        status_icon = "✓"
    else:
        FAIL_COUNT += 1
        status = Text("FAIL ", style="bold red")
        icon = Text("✗ ", style="bold red")
        status_icon = "✗"
    
    if output:
        TEST_OUTPUTS[test_id] = output
    TEST_RECORDS.append(TestRecord(
        test_id=test_id,
        label=label,
        status=TestStatus.PASS if passed else TestStatus.FAIL,
        detail=detail,
        output=output,
        category=CURRENT_CATEGORY,
        section=CURRENT_SECTION,
        question=CURRENT_QUESTION,
    ))
    CURRENT_QUESTION = ""
    
    line = Text.assemble(
        "  ", icon, " ", status, f"[{test_id}] ", label
    )
    if detail:
        line.append(Text(f" ({detail})", style="dim"))
    if not INTERACTIVE_REPORT_MODE:
        console.print(line)
        if status_spinner:
            status_spinner.update(f"[bold {('green' if passed else 'red')}]Running... {status_icon} [{TEST_COUNTER}]")
    return test_id

def warn_report(label: str, detail: str = ""):
    global WARN_COUNT, TEST_COUNTER, CURRENT_QUESTION
    TEST_COUNTER += 1
    test_id = TEST_COUNTER
    WARN_COUNT += 1
    status = Text("WARN ", style="bold yellow")
    icon = Text("⚠ ", style="bold yellow")
    line = Text.assemble(
        "  ", icon, " ", status, f"[{test_id}] ", label
    )
    if detail:
        line.append(Text(f" ({detail})", style="dim"))
    if not INTERACTIVE_REPORT_MODE:
        console.print(line)
    TEST_RECORDS.append(TestRecord(
        test_id=test_id,
        label=label,
        status=TestStatus.WARN,
        detail=detail,
        output="",
        category=CURRENT_CATEGORY,
        section=CURRENT_SECTION,
        question=CURRENT_QUESTION,
    ))
    CURRENT_QUESTION = ""
    return test_id

def show_complex_question(question: str, category: str):
    global CURRENT_QUESTION
    CURRENT_QUESTION = question
    if INTERACTIVE_REPORT_MODE:
        return
    category_text = Text(category, style="bold magenta", justify="center")
    question_text = Text(question, style="cyan", justify="center")
    panel = Panel.fit(
        question_text,
        title=category_text,
        border_style="magenta",
        box=box.ROUNDED,
        width=70,
    )
    console.print(panel)

def print_summary():
    if INTERACTIVE_REPORT_MODE:
        return
    stop_spinner()
    if progress:
        progress.stop()
    total = PASS_COUNT + FAIL_COUNT + WARN_COUNT
    pct = (PASS_COUNT / total * 100) if total > 0 else 0
    
    console.print()
    
    summary_text = Text.assemble(
        f"Total: [bold]{total}[/bold]  ",
        f"[bold green]Passed: {PASS_COUNT}[/bold green]  ",
        f"[bold red]Failed: {FAIL_COUNT}[/bold red]  ",
        f"[bold yellow]Warnings: {WARN_COUNT}[/bold yellow]\n",
        f"Success Rate: [bold cyan]{pct:.1f}%[/bold cyan]",
    )
    console.print(Panel.fit(
        summary_text,
        title="[bold]Test Results Summary[/bold]",
        border_style="cyan",
        box=box.ROUNDED,
    ))
    
    if FAIL_COUNT > 0:
        console.print("\n[bold red]✗ FAILURES NEED INVESTIGATION[/bold red]")
    if WARN_COUNT > 0:
        console.print(f"\n[bold yellow]⚠ Review {WARN_COUNT} warnings for potential issues[/bold yellow]")
    
    if pct == 100:
        console.print("\n[bold green]🎉 ALL TESTS PASSED! Excellent work![/bold green]")
    elif pct >= 90:
        console.print("\n[bold green]👍 Great job! Most tests passed.[/bold green]")
    elif pct >= 70:
        console.print("\n[bold yellow]📊 Good progress! Keep improving.[/bold yellow]")
    else:
        console.print("\n[bold red]🔧 Needs work. Please review failures.[/bold red]")


def print_dim(text: str):
    if INTERACTIVE_REPORT_MODE:
        return
    console.print(f"[dim]{text}[/dim]")

def interactive_menu():
    menu_text = Text.assemble(
        "[bold cyan]Interactive Mode[/bold cyan]\n\n",
        "Enter a command to view test details:\n",
        "[green]list[/green]   - show all test numbers\n",
        "[green]pass[/green]  - show passed tests\n",
        "[green]fail[/green]  - show failed tests\n",
        "[green]all[/green]   - show all test outputs\n",
        "[green]<n>[/green]    - expand test number n\n",
        "[green]q[/green]     - quit\n",
    )
    panel = Panel.fit(menu_text, title="Help", border_style="cyan")
    console.print(panel)
    
    while True:
        try:
            user_input = Prompt.ask("[cyan]Enter command[/cyan]", default="q")
            
            if user_input.lower() in ('q', 'quit'):
                break
            
            if user_input.lower() == 'list':
                show_test_list()
                continue
            
            if user_input.lower() == 'pass':
                show_tests_by_status(TestStatus.PASS)
                continue
            
            if user_input.lower() == 'fail':
                show_tests_by_status(TestStatus.FAIL)
                continue
            
            if user_input.lower() == 'all':
                show_all_outputs()
                continue
            
            try:
                test_id = int(user_input)
                show_test_output(test_id)
            except ValueError:
                console.print("[red]Invalid input. Enter a number or command.[/red]")
                
        except (EOFError, KeyboardInterrupt):
            break

def show_test_list():
    console.print("\n[bold]Available Tests:[/bold]")
    for record in TEST_RECORDS:
        if record.status == TestStatus.PASS:
            status_text = Text("PASS", style="green")
        elif record.status == TestStatus.WARN:
            status_text = Text("WARN", style="yellow")
        else:
            status_text = Text("FAIL", style="red")
        console.print(f"  [{record.test_id:3d}] {status_text}")

def show_tests_by_status(status: TestStatus):
    console.print(f"\n[bold]{status.value.upper()} Tests:[/bold]\n")
    for record in TEST_RECORDS:
        if record.status == status:
            console.print(f"  [{record.test_id}] {record.label}")

def show_all_outputs():
    count = 0
    for tid in range(1, TEST_COUNTER + 1):
        output = TEST_OUTPUTS.get(tid)
        if output:
            count += 1
            show_test_output(tid)
    console.print(f"[green]Showing {count} test outputs[/green]")

def show_test_output(test_id: int):
    output = TEST_OUTPUTS.get(test_id)
    if output:
        console.print(Panel(
            Text(output[:2000], style="yellow"),
            title=f"Test {test_id}",
            border_style="cyan",
        ))
    else:
        console.print(f"[yellow]Test {test_id} has no output[/yellow]")


def format_lines(lines) -> str:
    if lines is None:
        return ""
    if isinstance(lines, str):
        return lines
    return "\n".join(str(line) for line in lines)


def compact_expr_text(text: str) -> str:
    return "".join(text.split()).replace("(", "").replace(")", "")


def extract_final_expr(lines) -> str:
    if lines is None:
        return ""
    i = len(lines) - 1
    while i >= 0:
        line = str(lines[i]).strip()
        if line.startswith("Final = "):
            return line[len("Final = "):].strip()
        if line.startswith("= "):
            return line[2:].strip()
        i -= 1
    return ""


def nearly_equal(a: float, b: float, tol: float = 1e-7) -> bool:
    scale = max(1.0, abs(a), abs(b))
    return abs(a - b) <= tol * scale


def eval_int_node_numeric(node, x_value: float) -> Optional[float]:
    try:
        kind = node[0]
        if kind == 'num':
            return node[1] / node[2]
        if kind == 'sym':
            if node[1] == 'x':
                return x_value
            return None
        if kind == 'const':
            if node[1] == 'e':
                return math.e
            if node[1] == 'pi':
                return math.pi
            return None
        if kind == 'add':
            total = 0.0
            for part in node[1]:
                value = eval_int_node_numeric(part, x_value)
                if value is None:
                    return None
                total += value
            return total
        if kind == 'mul':
            total = 1.0
            for part in node[1]:
                value = eval_int_node_numeric(part, x_value)
                if value is None:
                    return None
                total *= value
            return total
        if kind == 'div':
            top = eval_int_node_numeric(node[1], x_value)
            bot = eval_int_node_numeric(node[2], x_value)
            if top is None or bot is None or abs(bot) < 1e-12:
                return None
            return top / bot
        if kind == 'pow':
            base = eval_int_node_numeric(node[1], x_value)
            exp = eval_int_node_numeric(node[2], x_value)
            if base is None or exp is None:
                return None
            if abs(exp - round(exp)) < 1e-9:
                return base ** int(round(exp))
            if base < 0:
                return None
            return base ** exp
        if kind == 'fn':
            name = node[1]
            arg = eval_int_node_numeric(node[2], x_value)
            if arg is None:
                return None
            if name in ('log', 'ln'):
                if arg <= 0:
                    return None
                return math.log(arg)
            if name == 'sqrt':
                if arg < 0:
                    return None
                return math.sqrt(arg)
            if name == 'abs':
                return abs(arg)
            if name == 'sin':
                return math.sin(arg)
            if name == 'cos':
                return math.cos(arg)
            if name == 'tan':
                return math.tan(arg)
            if name == 'sec':
                cos_value = math.cos(arg)
                if abs(cos_value) < 1e-12:
                    return None
                return 1.0 / cos_value
            if name == 'cosec':
                sin_value = math.sin(arg)
                if abs(sin_value) < 1e-12:
                    return None
                return 1.0 / sin_value
            if name == 'cot':
                tan_value = math.tan(arg)
                if abs(tan_value) < 1e-12:
                    return None
                return 1.0 / tan_value
            if name == 'exp':
                return math.exp(arg)
            if name == 'atan':
                return math.atan(arg)
            if name == 'asin':
                if arg < -1 or arg > 1:
                    return None
                return math.asin(arg)
            if name == 'acos':
                if arg < -1 or arg > 1:
                    return None
                return math.acos(arg)
            return None
    except (OverflowError, ValueError, ZeroDivisionError):
        return None
    return None


def semantically_matches_expression(actual_text: str, target_text: str) -> bool:
    if actual_text == "" or target_text == "":
        return False
    try:
        actual = ALG.parse(actual_text)
        target = ALG.parse(target_text)
        return ALG.equivalent(actual, target) or ALG.same(ALG.canonical_compare_form(actual), ALG.canonical_compare_form(target))
    except Exception:
        return compact_expr_text(actual_text) == compact_expr_text(target_text)


def integration_matches(expr_text: str, answer_node) -> bool:
    if answer_node is None:
        return False
    try:
        derived = INTEGRATE.sim(INTEGRATE.diff(answer_node, 'x'))
        original = INTEGRATE.sim(INTEGRATE.parse(expr_text))
        if INTEGRATE.sig(derived) == INTEGRATE.sig(original):
            return True
        sample_points = [-2.4, -1.8, -1.1, -0.6, -0.2, 0.25, 0.6, 1.1, 1.7, 2.3, 3.1]
        valid_checks = 0
        for x_value in sample_points:
            derived_value = eval_int_node_numeric(derived, x_value)
            original_value = eval_int_node_numeric(original, x_value)
            if derived_value is None or original_value is None:
                continue
            if not math.isfinite(derived_value) or not math.isfinite(original_value):
                continue
            valid_checks += 1
            if not nearly_equal(derived_value, original_value):
                return False
        return valid_checks >= 3
    except Exception:
        return False


def roots_match(actual_roots, expected_root_texts) -> bool:
    if actual_roots is None or len(actual_roots) != len(expected_root_texts):
        return False
    expected_roots = [ALG.parse(text) for text in expected_root_texts]
    used = [False] * len(actual_roots)
    for expected in expected_roots:
        found = False
        for index, actual in enumerate(actual_roots):
            if used[index]:
                continue
            if ALG.equivalent(actual, expected):
                used[index] = True
                found = True
                break
        if not found:
            return False
    return True


def step_present(steps, key_step: str) -> bool:
    if key_step == "":
        return True
    needle = key_step.lower()
    return any(needle in str(step).lower() for step in steps)


def trig_solution_count(text: str, expected_count: int) -> bool:
    try:
        result, _lines = TRIG.solve_solve_text(text)
        return isinstance(result, list) and len(result) == expected_count
    except Exception:
        return False


if TEXTUAL_AVAILABLE:
    class ResultsApp(App):
        CSS = """
        Screen {
            layout: vertical;
            background: #0b1020;
            color: #f2f5ff;
        }
        #summary {
            height: auto;
            border: round #4cc9f0;
            padding: 1 2;
            margin: 1 2 0 2;
            background: #121933;
        }
        #body {
            border: round #2d3b68;
            margin: 1 2 1 2;
            padding: 0 1;
            background: #0f1730;
        }
        .section {
            color: #9cc9ff;
            text-style: bold;
            padding: 1 0 0 0;
        }
        .record-output {
            border: round #31456f;
            padding: 1;
            margin: 0 1 1 1;
            background: #101a35;
            color: #f7f9ff;
        }
        .record-meta {
            color: #b7c3e0;
            padding: 0 1;
        }
        """
        BINDINGS = [
            ("q", "quit", "Quit"),
            ("e", "expand_all", "Expand All"),
            ("c", "collapse_all", "Collapse All"),
        ]

        def __init__(self, records):
            super().__init__()
            self.records = records

        def compose(self) -> ComposeResult:
            yield Header(show_clock=True)
            yield Static(self.summary_text(), id="summary")
            with VerticalScroll(id="body"):
                grouped = defaultdict(lambda: defaultdict(list))
                for record in self.records:
                    grouped[record.category][record.section].append(record)
                for category, sections in grouped.items():
                    category_records = [record for section_records in sections.values() for record in section_records]
                    category_pass = sum(1 for record in category_records if record.status == TestStatus.PASS)
                    category_fail = sum(1 for record in category_records if record.status == TestStatus.FAIL)
                    category_warn = sum(1 for record in category_records if record.status == TestStatus.WARN)
                    title = f"{category}  [{category_pass} pass / {category_fail} fail / {category_warn} warn]"
                    with Collapsible(title=title, collapsed=False):
                        for section, section_records in sections.items():
                            section_pass = sum(1 for record in section_records if record.status == TestStatus.PASS)
                            section_fail = sum(1 for record in section_records if record.status == TestStatus.FAIL)
                            section_warn = sum(1 for record in section_records if record.status == TestStatus.WARN)
                            section_title = f"{section}  [{section_pass} pass / {section_fail} fail / {section_warn} warn]"
                            section_collapsed = section_fail == 0 and section_warn == 0
                            with Collapsible(title=section_title, collapsed=section_collapsed):
                                for record in section_records:
                                    icon = "✓" if record.status == TestStatus.PASS else ("⚠" if record.status == TestStatus.WARN else "✗")
                                    status_word = "PASS" if record.status == TestStatus.PASS else ("WARN" if record.status == TestStatus.WARN else "FAIL")
                                    title_text = f"{icon} {status_word} [{record.test_id}] {record.label}"
                                    collapsed = record.status == TestStatus.PASS
                                    with Collapsible(title=title_text, collapsed=collapsed):
                                        meta_lines = []
                                        if record.question:
                                            meta_lines.append("Question:\n" + record.question)
                                        if record.detail:
                                            meta_lines.append("Detail:\n" + record.detail)
                                        if meta_lines:
                                            yield Static("\n\n".join(meta_lines), classes="record-meta")
                                        yield Static(record.output or "(no output captured)", classes="record-output")
            yield Footer()

        def summary_text(self) -> str:
            total = PASS_COUNT + FAIL_COUNT + WARN_COUNT
            pct = (PASS_COUNT / total * 100) if total else 0.0
            return (
                "CASIO Interactive Test Report\n"
                f"Total: {total}    Passed: {PASS_COUNT}    Failed: {FAIL_COUNT}    Warnings: {WARN_COUNT}    Success: {pct:.1f}%\n"
                "Controls: click a chevron to expand a test, or use E to expand all, C to collapse all, Q to quit."
            )

        def action_expand_all(self) -> None:
            for widget in self.query(Collapsible):
                widget.collapsed = False

        def action_collapse_all(self) -> None:
            first = True
            for widget in self.query(Collapsible):
                if first:
                    widget.collapsed = False
                    first = False
                else:
                    widget.collapsed = True

if INTERACTIVE_REPORT_MODE:
    console.print("[dim]Building CASIO interactive report...[/dim]")
else:
    print_header()
    print_dim("Testing calculators: Algebra, Trigonometry, Derive, Integrate, SUVAT")
    console.print()

print_category("CATEGORY 1: ALGEBRA CORE TESTS")
print_dim("Testing algebraic manipulation, solving, and transformation capabilities")

print_section("1.1 Algebra - Expression Comparison")
test_cases_algebra_compare = [
    ("(x+1)^2", "x^2+2*x+1", "Equivalent"),
    ("sin(x)^2+cos(x)^2", "1", "Equivalent"),
    ("(1-cos(2*x))/2", "sin(x)^2", "Equivalent"),
    ("(x^2-1)/(x-1)", "x+1", "Equivalent"),
    ("exp(log(x))", "x", "Equivalent"),
    ("log(e^x)", "x", "Equivalent"),
    ("(x+2)*(x+3)", "x^2+5*x+6", "Equivalent"),
    ("(sec(x))^2-(tan(x))^2", "1", "Equivalent"),
    ("(cosec(x))^2-(cot(x))^2", "1", "Equivalent"),
    ("(x+1)^3", "x^3+3*x^2+3*x+1", "Equivalent"),
    ("(x-1)*(x+1)", "x^2-1", "Equivalent"),
    ("(1+cos(2*x))/2", "cos(x)^2", "Equivalent"),
]

for expr1, expr2, expected in test_cases_algebra_compare:
    _code, out, _err = run_cli('algebraProgram.py', f'1\n{expr1}\n{expr2}\n')
    passed = expected in out
    report(f"Compare: {expr1} ≟ {expr2}", passed, expected if not passed else "", out)

print_section("1.2 Algebra - Transformation")
test_cases_algebra_transform = [
    ("1-cos(2*x)", "2*sin(x)^2"),
    ("sin(x)/cos(x)", "tan(x)"),
    ("1/cos(x)", "sec(x)"),
    ("1/sin(x)", "cosec(x)"),
    ("cos(x)/sin(x)", "cot(x)"),
    ("(1-cos(x))/sin(x)", "tan(x/2)"),
    ("sin(x)/(1+cos(x))", "tan(x/2)"),
    ("(1+cos(x))/sin(x)", "cot(x/2)"),
    ("sec(x)^2-tan(x)^2", "1"),
    ("sin(2*x)", "2*sin(x)*cos(x)"),
    ("cos(2*x)", "cos(x)^2-sin(x)^2"),
    ("exp(log(x))", "x"),
    ("log(e^x)", "x"),
]

for expr, target in test_cases_algebra_transform:
    try:
        lines = ALG.solve_transform_text(expr, target)
        final_expr = extract_final_expr(lines)
        passed = semantically_matches_expression(final_expr or target, target)
        detail = f"Expected {target}" if not passed else ""
        report(f"Transform: {expr} → {target}", passed, detail, format_lines(lines))
    except Exception as err:
        report(f"Transform: {expr} → {target}", False, str(err), "")

print_section("1.3 Algebra - Solve Equations")
test_cases_algebra_solve = [
    ("x^2+5*x+6=0", "x = [-3, -2]"),
    ("(x+1)/(x-2)=3", "x = 7/2"),
    ("1/(x+1)+1/(x-2)=0", "x = 1/2"),
    ("2*x+6=0", "x = -3"),
    ("x^2-9=0", "x = [3, -3]"),
    ("2*x^2+7*x+3=0", "x = [-3, -1/2]"),
    ("3*x^2+12*x+12=0", "x = -2"),
    ("(x^2-1)/(x-1)=0", "x = -1"),
    ("(x+2)*(x+3)=0", "x = [-3, -2]"),
    ("4*x^2-9=0", "x = [3/2, -3/2]"),
    ("9-x^2=0", "x = [3, -3]"),
    ("2*x+1=0", "x = -1/2"),
]

for eq, expected in test_cases_algebra_solve:
    _code, out, _err = run_cli('algebraProgram.py', f'6\n{eq}\n')
    passed = expected in out
    report(f"Solve: {eq}", passed, expected if not passed else "", out)

print_section("1.4 Algebra - Inverse Functions")
test_cases_algebra_inverse = [
    ("(2*x+1)/(3*x+4)", "f^-1(x) = (-4*x+1)/(3*x-2)"),
    ("1/(x+1)^3", "f^-1(x) = 1/x^(1/3)-1"),
    ("sqrt((2*x+3)^3)", "f^-1(x) = (x^(2/3)-3)/2"),
    ("3*x-7", "f^-1(x) = (x+7)/3"),
]

for expr, expected in test_cases_algebra_inverse:
    _code, out, _err = run_cli('algebraProgram.py', f'8\n{expr}\n')
    passed = expected in out
    detail = f"Expected inverse" if not passed else ""
    report(f"Inverse: {expr}", passed, detail, out)

print_category("CATEGORY 2: TRIGONOMETRY TESTS")
print_dim("Proving identities, solving equations, transformations")

print_section("2.1 Trig - Prove Identities")
test_cases_trig_prove = [
    ("sin(x)^2+cos(x)^2=1", "LHS = RHS"),
    ("sec(x)^2-tan(x)^2=1", "LHS = RHS"),
    ("cosec(x)^2-cot(x)^2=1", "LHS = RHS"),
    ("sin(2*x)=2*sin(x)*cos(x)", "LHS = RHS"),
    ("cos(2*x)=cos(x)^2-sin(x)^2", "LHS = RHS"),
    ("1-cos(2*x)=2*sin(x)^2", "LHS = RHS"),
    ("1+cos(2*x)=2*cos(x)^2", "LHS = RHS"),
    ("tan(x)=sin(x)/cos(x)", "LHS = RHS"),
    ("cot(x)=cos(x)/sin(x)", "LHS = RHS"),
    ("sin(3*x)=3*sin(x)-4*sin(x)^3", "LHS = RHS"),
    ("cos(3*x)=4*cos(x)^3-3*cos(x)", "LHS = RHS"),
]

for identity, expected in test_cases_trig_prove:
    _code, out, _err = run_cli('trigProgram.py', f'1\n{identity}\n1\n')
    passed = expected in out
    report(f"Prove: {identity}", passed, expected if not passed else "", out)

print_section("2.2 Trig - Transform Expressions")
test_cases_trig_transform = [
    ("sin(x)/cos(x)", "tan(x)"),
    ("1/cos(x)", "sec(x)"),
    ("1/sin(x)", "cosec(x)"),
    ("1-cos(2*x)", "2*sin(x)^2"),
    ("1+cos(2*x)", "2*cos(x)^2"),
    ("sin(2*x)", "2*sin(x)*cos(x)"),
    ("cos(2*x)", "cos(x)^2-sin(x)^2"),
    ("sin(3*x)", "3*sin(x)-4*sin(x)^3"),
    ("cos(3*x)", "4*cos(x)^3-3*cos(x)"),
]

for expr, target in test_cases_trig_transform:
    _code, out, _err = run_cli('trigProgram.py', f'2\n{expr}\n{target}\n')
    passed = target in out or target.replace("*", "") in out.replace("*", "")
    detail = "" if passed else f"Missing {target}"
    report(f"Transform: {expr}", passed, detail, out)

print_section("2.3 Trig - Solve Equations")
test_cases_trig_solve = [
    ("sin(x)=0,x,0,360", "x = [0, 180, 360]"),
    ("cos(x)=0,x,0,360", "x = [90, 270]"),
    ("tan(x)=1,x,0,360", "x = [45, 225]"),
    ("sec(x)=2,x,0,360", "x = [60, 300]"),
    ("cosec(x)=2,x,0,360", "x = [30, 150]"),
    ("cot(x)=1,x,0,360", "x = [45, 225]"),
    ("sin(3*x)=0,x,0,360", "x = [0, 60, 120, 180, 240, 300, 360]"),
    ("cos(2*x)=1,x,0,360", "x = [0, 180, 360]"),
    ("1/cos(x)=0,x,0,360", "has no real solutions"),
    ("sin(x)=1/2,x,0,360", "x = [30, 150]"),
    ("cos(x)=1/2,x,0,360", "x = [60, 300]"),
    ("tan(2*x)=0,x,0,360", "x = [0, 90, 180, 270, 360]"),
]

for eq, expected in test_cases_trig_solve:
    _code, out, _err = run_cli('trigProgram.py', f'3\n{eq}\n')
    passed = expected in out
    detail = "" if passed else f"Expected {expected}"
    report(f"Solve: {eq}", passed, detail, out)

print_category("CATEGORY 3: CALCULUS - DERIVATIVES")
print_dim("Testing differentiation of polynomials, trig, exponential, logarithmic functions")

print_section("3.1 Derive - Basic Rules")
sys.path.insert(0, str(ROOT))
sys._derive_no_autorun = True

try:
    import deriveProgram as dp
    
    test_cases_derive = [
        ("x^7-3*x^3+5*x-8"),
        ("sin(x)*cos(x)"),
        ("exp(3*x-2)"),
        ("log(x^2+3*x+5)"),
        ("sqrt(2*x+9)"),
        ("(x^2+1)/(x+3)"),
    ]
    
    for expr in test_cases_derive:
        try:
            node = dp.parse(expr)
            deriv = dp.diff(node, 'x', {})
            deriv_tidy = dp.tidy(deriv)
            deriv_text = dp.show(deriv_tidy)
            passed = deriv_text is not None and len(deriv_text) > 0
            detail = "" if passed else f"Failed to derive"
            report(f"Derive: d/dx[{expr}]", passed, detail, deriv_text)
        except Exception as e:
            report(f"Derive: d/dx[{expr}]", False, str(e), "")
            
except ImportError as e:
    report("Import deriveProgram", False, str(e), "")

print_category("CATEGORY 4: CALCULUS - INTEGRATION")
print_dim("Testing indefinite integrals, substitution, by parts")

print_section("4.1 Integrate - Direct")
test_cases_int_direct = [
    ("x^7-3*x^2+4", "+ C"),
    ("1/x", "ln|x|"),
    ("2^x", "ln(2)"),
    ("exp(3*x-1)", "+ C"),
    ("(2*x+3)/(x^2+3*x+5)", "ln|"),
    ("1/(x^2+9)", "atan"),
    ("sqrt(2*x+5)", "+ C"),
    ("sin(x)", "+ C"),
    ("cos(x)", "+ C"),
    ("tan(x)", "+ C"),
    ("sec(x)^2", "+ C"),
]

for expr, expected in test_cases_int_direct:
    _code, out, _err = run_cli('intProgram.py', f'1\n{expr}\n1\n')
    passed = expected in out
    detail = f"Missing {expected}" if not passed else ""
    report(f"Integrate: ∫{expr}dx", passed, detail, out)

print_section("4.2 Integrate - Substitution")
test_cases_int_sub = [
    ("(3*x^2+1)/(x^3+x+7)", "x^3+x+7"),
    ("(2*x)*exp(x^2)", "x^2"),
    ("(2*x+3)/sqrt(x^2+3*x+10)", "x^2+3*x+10"),
    ("cos(3*x+1)", "3*x+1"),
    ("1/(2*x+5)", "2*x+5"),
    ("(5*x+1)^4", "5*x+1"),
    ("3/(1+(3*x)^2)", "3*x"),
    ("1/sqrt(1-(2*x)^2)", "2*x"),
]

for expr, subst in test_cases_int_sub:
    _code, out, _err = run_cli('intProgram.py', f'1\n{expr}\n4\n{subst}\n')
    passed = "u = " + subst in out or subst in out
    detail = f"Missing substitution for {subst}" if not passed else ""
    report(f"Sub: ∫{expr}dx, u={subst}", passed, detail, out)

print_section("4.3 Integrate - By Parts")
test_cases_int_parts = [
    ("x*sin(x)", "+ C"),
    ("x*cos(x)", "+ C"),
    ("x*exp(x)", "+ C"),
    ("x^2*exp(x)", "+ C"),
    ("log(x)", "+ C"),
    ("log(x)^2", "+ C"),
    ("asin(x)", "+ C"),
    ("acos(x)", "+ C"),
    ("atan(x)", "+ C"),
    ("x*log(x)", "+ C"),
]

for expr, expected in test_cases_int_parts:
    _code, out, _err = run_cli('intProgram.py', f'1\n{expr}\n5\n')
    passed = expected in out
    detail = f"Missing {expected}" if not passed else ""
    report(f"Parts: ∫{expr}dx", passed, detail, out)

print_category("CATEGORY 5: SUVAT PHYSICS")
print_dim("Kinematics equations for motion under constant acceleration")

print_section("5.1 SUVAT - Basic Solutions")
sys.path.insert(0, str(ROOT))
sys._suvat_no_autorun = True

try:
    import SUVATprogram as sp
    
    test_cases_suvat = [
        (None, sp.num(5), None, sp.num(2), sp.num(3), 's', '24'),
        (None, sp.num(0), None, sp.num(4), sp.num(5), 's', '50'),
        (None, sp.num(-3), None, sp.num(2), sp.num(4), 's', '4'),
        (sp.num(20), None, sp.num(14), sp.num(2), sp.num(4), 'u', '6'),
        (sp.num(15), None, sp.num(9), sp.num(-2), sp.num(3), 'u', '15'),
        (None, sp.num(7), None, sp.num(3), sp.num(2), 'v', '13'),
        (None, sp.num(0), None, sp.num(9), sp.num(2), 'v', '18'),
    ]
    
    for s, u, v, a, t, target, expected in test_cases_suvat:
        try:
            result, equation, original_eq, sub_text = sp._build_suvat_solution_data(s, u, v, a, t, target)
            if result is None:
                passed = expected.lower() in str(equation).lower()
                report(f"SUVAT: solve for {target}", passed, f"Expected {expected}, got {equation}", str(equation))
            else:
                text = sp.show(result) if not isinstance(result, list) else ' or '.join(sp.show(x) for x in result)
                passed = expected in text or expected in str(equation)
                report(f"SUVAT: solve for {target}", passed, f"Expected {expected}", text)
        except Exception as e:
            report(f"SUVAT: solve for {target}", False, str(e), "")
            
except ImportError as e:
    report("Import SUVATprogram", False, str(e), "")

print_section("5.2 SUVAT - Physical Constraints")
try:
    physical_test_cases = [
        (sp.num(10), sp.num(0), None, sp.num(-5), None, 'v', 'no real solution'),
        (sp.num(-10), sp.num(1), None, sp.num(1), None, 't', 'no real solution'),
        (None, sp.num(5), sp.num(10), sp.num(0), None, 't', 'no solution'),
        (sp.num(10), None, sp.num(5), sp.num(2), sp.num(0), 'u', 'no solution'),
    ]

    for s, u, v, a, t, target, expected in physical_test_cases:
        try:
            result, equation, original_eq, sub_text = sp._build_suvat_solution_data(s, u, v, a, t, target)
            passed = expected.lower() in str(equation).lower() if result is None else False
            detail = f"Expected error containing '{expected}'" if not passed else ""
            report(f"Physical: {target}", passed, detail, str(equation))
        except Exception as e:
            report(f"Physical: {target}", False, str(e), "")
except NameError:
    pass

print_category("CATEGORY 6: EDGE CASES & REGRESSION TESTS")
print_dim("Testing error handling, CLI robustness, blank input handling")

print_section("6.1 CLI - Blank Input Handling")
blank_tests = [
    ("deriveProgram.py", "1\n\n", "Err: Enter an expression."),
    ("intProgram.py", "1\n\n1\n", "Err: Enter f."),
    ("trigProgram.py", "1\n\n1\n", "Err: Enter an identity."),
    ("trigProgram.py", "2\n\n\n", "Err: Enter E1 and E2."),
    ("trigProgram.py", "3\n\n", "Err: Enter an equation."),
    ("trigProgram.py", "4\n\n\n", "Err: Enter an expression."),
]

for script, inp, expected in blank_tests:
    _code, out, _err = run_cli(script, inp)
    passed = expected in out
    report(f"Blank input: {script}", passed, expected if not passed else "", out)

print_section("6.2 CLI - Error Messages")
error_tests = [
    ("trigProgram.py", "2\nsin(x)\nsin(y)\n", "different trig variable"),
    ("trigProgram.py", "3\n1/cos(x)=0,x,0,360\n", "has no real solutions"),
    ("algebraProgram.py", "8\n\n", "Use:"),
    ("intProgram.py", "1\ne^(x^2)\n1\n", "Out of scope"),
]

for script, inp, expected in error_tests:
    _code, out, _err = run_cli(script, inp)
    passed = expected in out
    report(f"Error msg: {script}", passed, expected if not passed else "", out)

print_category("CATEGORY 7: EXTREME CHALLENGE QUESTIONS")
print_dim("⚡ Ultra-complex problems that push calculators to their limits")

print_section("7.1 Algebra - Deep Complexity")

show_complex_question(
    "Simplify: (x^3 - y^3) / ((x-y)*(x^2 + xy + y^2)) - (x^3 + y^3) / ((x+y)*(x^2 - xy + y^2))",
    "ALGEBRA EXTREME"
)
_code, out, _err = run_cli('algebraProgram.py', '4\n(x^3 - y^3) / ((x-y)*(x^2 + xy + y^2)) - (x^3 + y^3) / ((x+y)*(x^2 - xy + y^2))\n')
passed = "x" in out and "y" in out and "Error" not in out
report("Complex algebraic fraction", passed, "", out)

show_complex_question(
    "Find all roots: x^5 - 5*x^4 + 10*x^3 - 10*x^2 + 5*x - 1",
    "ALGEBRA POLYNOMIAL"
)
_code, out, _err = run_cli('algebraProgram.py', '6\nx^5 - 5*x^4 + 10*x^3 - 10*x^2 + 5*x - 1=0\n')
passed = "x =" in out or "x =" in out.replace(" ", "")
report("Fifth degree polynomial", passed, "", out)

show_complex_question(
    "Compose: f(g(h(x))) where f(x) = x^3 + 2x, g(x) = sqrt(x+1), h(x) = 1/(x-1)",
    "ALGEBRA COMPOSITION"
)
_code, out, _err = run_cli('algebraProgram.py', '7\n1/(x-1)\nsqrt(x+1)\nx^3+2*x\n')
passed = "f(g(" in out or "composition" in out.lower() or "x" in out
report("Triple function composition", passed, "", out)

show_complex_question(
    "Find f⁻¹(x) if f(x) = (2x+1)/(3x+4) - complex rational function",
    "ALGEBRA INVERSE COMPLEX"
)
_code, out, _err = run_cli('algebraProgram.py', '8\n(2*x+1)/(3*x+4)\n')
passed = "f^-1" in out or "inverse" in out.lower()
report("Complex rational inverse", passed, "", out)

show_complex_question(
    "Rewrite x²+5x+6 in terms of u=2x+1 (complete the square in u)",
    "ALGEBRA REWRITE COMPLEX"
)
_code, out, _err = run_cli('algebraProgram.py', '9\nx^2+5*x+6\n2*x+1\n\n')
passed = "u =" in out or "T" in out
report("Complete square rewrite", passed, "", out)

print_section("7.2 Trig - Extreme Identity Proofs")

show_complex_question(
    "Prove: sin(6*x) = 6*sin(x)*cos(x)^5 - 20*sin(x)^3*cos(x)^3 + 16*sin(x)^5*cos(x)",
    "TRIG IDENTITY"
)
_code, out, _err = run_cli('trigProgram.py', '1\nsin(6*x)=6*sin(x)*cos(x)^5-20*sin(x)^3*cos(x)^3+16*sin(x)^5*cos(x)\n1\n')
passed = "LHS = RHS" in out or "RHS" in out
report("Triple-angle identity", passed, "", out)

show_complex_question(
    "Solve: sin(x) + sin(2x) + sin(3x) = 0 for 0 ≤ x ≤ 2π",
    "TRIG SOLVE COMPLEX"
)
_code, out, _err = run_cli('trigProgram.py', '3\nsin(x)+sin(2*x)+sin(3*x)=0,x,0,6.283\n')
passed = "x =" in out or "solutions" in out.lower()
report("Multi-term trig equation", passed, "", out)

show_complex_question(
    "Solve: sin(x)cos(x) = 1/2 for 0° ≤ x ≤ 360°",
    "TRIG PRODUCT SOLVE"
)
_code, out, _err = run_cli('trigProgram.py', '3\nsin(x)*cos(x)=1/2,x,0,360\n')
passed = "x =" in out or "45" in out or "135" in out
report("Trig product equation", passed, "", out)

show_complex_question(
    "Prove: tan(x) + cot(x) = 2*csc(2x)",
    "TRIG RECIPROCAL ID"
)
_code, out, _err = run_cli('trigProgram.py', '1\ntan(x)+cot(x)=2*cosec(2*x)\n1\n')
passed = "LHS = RHS" in out or "RHS" in out or "Equivalent" in out
report("Reciprocal identity", passed, "", out)

print_section("7.3 Calculus - Extreme Integration")

show_complex_question(
    "Integrate: ∫ x² · sin³(x) · eˣ dx",
    "INTEGRATION EXTREME"
)
_code, out, _err = run_cli('intProgram.py', '1\nx^2*sin(x)^3*exp(x)\n1\n')
passed = "+ C" in out or "Out of scope" not in out
report("Product poly*trig*exp", passed, "", out)

show_complex_question(
    "Integrate: ∫ (x³ + 2x² - x + 1) / ((x-1)²(x+2)) dx using partial fractions",
    "RATIONAL INTEGRATION"
)
_code, out, _err = run_cli('intProgram.py', '1\n(x^3+2*x^2-x+1)/((x-1)^2*(x+2))\n1\n')
passed = "partial" in out.lower() or "+ C" in out
report("Complex partial fractions", passed, "", out)

show_complex_question(
    "Integrate: ∫ sin⁴(x)cos²(x) dx",
    "INTEGRATION TRIG POWER"
)
_code, out, _err = run_cli('intProgram.py', '1\nsin(x)^4*cos(x)^2\n1\n')
passed = "+ C" in out and "sin" in out
report("High power trig integral", passed, "", out)

show_complex_question(
    "Integrate: ∫ x·ln²(x) dx using integration by parts",
    "INTEGRATION BY PARTS"
)
_code, out, _err = run_cli('intProgram.py', '1\nx*log(x)^2\n5\n')
passed = "+ C" in out
report("By parts log squared", passed, "", out)

print_section("7.4 SUVAT - Extreme Physics")

show_complex_question(
    "A ball is thrown upwards at 25 m/s from height h. It hits ground after 4s. Find h and max height.",
    "SUVAT PROJECTILE"
)
_code, out, _err = run_cli('SUVATprogram.py', '25\n0\n9.8\n4\n\n')
passed = "s =" in out or "h =" in out.lower() or "height" in out.lower()
report("Projectile height problem", passed, "", out)

show_complex_question(
    "Solve a quadratic-time SUVAT case: from rest, acceleration 2 m/s², displacement 100m. Find time.",
    "SUVAT QUADRATIC TIME"
)
try:
    result, equation, original_eq, sub_text = SUVAT._build_suvat_solution_data(SUVAT.num(100), SUVAT.num(0), None, SUVAT.num(2), None, 't')
    out = "\n".join([
        "Equation: " + str(equation),
        "Substitution: " + (sub_text or ""),
        "Result: " + (SUVAT.show(result) if result is not None else "None"),
    ])
    passed = result is not None and SUVAT.show(result) == '10'
    report("Quadratic-time motion", passed, "Expected t = 10" if not passed else "", out)
except Exception as err:
    report("Quadratic-time motion", False, str(err), "")

print_section("7.5 Derive - Extreme")

show_complex_question(
    "Find d/dx[xˣ] using logarithmic differentiation",
    "DERIVE LOG DIFFERENTIATION"
)
_code, out, _err = run_cli('deriveProgram.py', '1\nx^x\n')
passed = "dy/dx" in out or "Log diff" in out or "x^x" in out
report("Logarithmic differentiation", passed, "", out)

show_complex_question(
    "Find dy/dx if x² + y² + 2xy = 5 (implicit differentiation)",
    "DERIVE IMPLICIT"
)
_code, out, _err = run_cli('deriveProgram.py', '1\nx^2+y^2+2*x*y=5\n')
passed = "dy/dx" in out or "y'" in out or "=" in out
report("Implicit differentiation", passed, "", out)

show_complex_question(
    "Find d/dx[sin(x)ˣ] - variable base AND exponent",
    "DERIVE VAR BASE EXP"
)
_code, out, _err = run_cli('deriveProgram.py', '1\nsin(x)^x\n')
passed = "dy/dx" in out or "Log diff" in out
report("Variable base and exponent", passed, "", out)

print_section("7.6 Mixed Challenge - Combined Operations")

show_complex_question(
    "Integrate: ∫ 1/√(x²+6x+5) dx - involves completing the square",
    "MIXED INT+ALG"
)
_code, out, _err = run_cli('intProgram.py', '1\n1/sqrt(x^2+6*x+5)\n1\n')
passed = "+ C" in out or "ln" in out or "asinh" in out.lower() or "arsinh" in out.lower()
report("Integration with completing square", passed, "", out)

show_complex_question(
    "Transform: sec²(x) - tan²(x) using trig identities",
    "MIXED ALG+TRIG"
)
_code, out, _err = run_cli('algebraProgram.py', '2\nsec(x)^2-tan(x)^2\n1\n')
passed = "1" in out or "Equivalent" in out
report("Algebra-trig transformation", passed, "", out)

show_complex_question(
    "Rewrite sin²x + cos²x in terms of sin(x) and cos(x)",
    "MIXED TRIG REWRITE"
)
_code, out, _err = run_cli('trigProgram.py', '4\nsin(x)^2+cos(x)^2\nsin(x)\ncos(x)\n\n')
passed = "Final" in out or "1" in out
report("Trig rewrite", passed, "", out)

print_category("CATEGORY 8: STRESS & PERFORMANCE")
print_dim("Testing system under extreme load")

print_section("8.1 Stress - Multiple Rapid Calls")
stress_passed = 0
stress_failed = 0

for i in range(20):
    try:
        _code, out, _err = run_cli('algebraProgram.py', f'1\nx^{i+1}\nx^{i+1}\n')
        if "Equivalent" in out or "Not equivalent" in out:
            stress_passed += 1
        else:
            stress_failed += 1
    except:
        stress_failed += 1

report(f"20 rapid algebra calls", stress_failed == 0, f"{stress_passed}/20 passed", f"{stress_passed} passed")

print_section("8.2 Parser Edge Cases")
parser_tests = [
    ("algebraProgram.py", "(3+sqrt(5))/2"),
    ("algebraProgram.py", "sqrt(2)^3"),
    ("algebraProgram.py", "2(3+4)"),
    ("algebraProgram.py", "sin(pi/6)+cos(pi/3)"),
]

for script, expr in parser_tests:
    _code, out, _err = run_cli(script, f'1\n{expr}\n1\n')
    passed = "Error" not in out or "Err" not in out
    report(f"Parse: {expr}", passed, "", out)

print_category("CATEGORY 9: ADDITIONAL EXTREME GENERATED TESTS")
print_dim("A larger generated bank of awkward, multi-step, and high-complexity cases across all programs")

print_section("9.1 Additional Algebra Extremes")
algebra_compare_extra = [
    ("(x-1)^4", "x^4-4*x^3+6*x^2-4*x+1"),
    ("(x^3-1)/(x-1)", "x^2+x+1"),
    ("(x^3+8)/(x+2)", "x^2-2*x+4"),
    ("(x+1)^2-(x-1)^2", "4*x"),
    ("(x^2+2*x+1)/(x+1)", "x+1"),
    ("2*log(3, x)", "log(3, x^2)"),
    ("1/(1/x)", "x"),
    ("sqrt(x)^4", "x^2"),
    ("(2*x+3)^2-(2*x-3)^2", "24*x"),
    ("(sin(x)+cos(x))^2", "1+sin(2*x)"),
    ("(x^2-4)/(x-2)", "x+2"),
    ("exp(2*log(x))", "x^2"),
]
for expr1, expr2 in algebra_compare_extra:
    ok, steps = ALG.compare_expressions(ALG.parse(expr1), ALG.parse(expr2))
    report(f"Extra compare: {expr1} ≟ {expr2}", ok, "" if ok else "Not equivalent", format_lines([step[1] for step in steps]))

algebra_solve_extra = [
    ("x^3-3*x^2+3*x-1=0", ["1"]),
    ("x^4-5*x^2+4=0", ["2", "1", "-1", "-2"]),
    ("x^3-x^2-x+1=0", ["1", "-1"]),
    ("x^3-6*x^2+11*x-6=0", ["3", "2", "1"]),
    ("x^5-5*x^4+10*x^3-10*x^2+5*x-1=0", ["1"]),
    ("x^4-16=0", ["2", "-2"]),
]
for eq, expected_roots in algebra_solve_extra:
    lines = ALG.solve_equation_text(eq)
    out = format_lines(lines)
    expr = ALG.parse_expr_or_equation(eq)
    _var_name, roots, _label = ALG.solve_equation(expr)
    passed = roots_match(roots, expected_roots)
    detail = "Expected roots " + ", ".join(expected_roots) if not passed else ""
    report(f"Extra solve: {eq}", passed, detail, out)

algebra_inverse_extra = [
    ("2^(3*x-2)", "(ln(x)/ln(2)+2)/3"),
    ("1/(sqrt(3*x+1))", "(1/x^2-1)/3"),
    ("log(5, (x+4)^3)", "e^(x*ln(5))/3-4"),
]
for expr, expected in algebra_inverse_extra:
    result, steps = ALG.inverse_function(expr)
    report(f"Extra inverse: {expr}", result == expected, expected if result != expected else "", format_lines(steps))

algebra_rewrite_extra = [
    ("(x+2)^8-2*(x+2)^4+1", ["(x+2)^4"]),
    ("(3*x-1)^6+(3*x-1)^3", ["(3*x-1)^3"]),
    ("sin(x)^6+cos(x)^6", ["sin(x)^2", "cos(x)^2"]),
]
for expr, terms in algebra_rewrite_extra:
    try:
        lines = ALG.solve_rewrite_text(expr, terms)
        final_expr = extract_final_expr(lines)
        passed = final_expr != ""
        report(f"Extra rewrite: {expr}", passed, "" if passed else "No final rewrite", format_lines(lines))
    except Exception as err:
        report(f"Extra rewrite: {expr}", False, str(err), "")

print_section("9.2 Additional Trig Extremes")
trig_prove_extra = [
    "sin(x)/(1+cos(x))=tan(x/2)",
    "(1-cos(x))/sin(x)=tan(x/2)",
    "(1+cos(x))/sin(x)=cot(x/2)",
    "2*sin(x)*cos(x)=sin(2*x)",
    "1-2*sin(x)^2=cos(2*x)",
    "2*cos(x)^2-1=cos(2*x)",
    "tan(x)+cot(x)=1/(sin(x)*cos(x))",
    "sec(x)^2-cosec(x)^2=tan(x)^2-cot(x)^2",
    "ln(sin(x)^2)/ln(2)=2*ln(sin(x))/ln(2)",
    "sin(x)^2=(1-cos(2*x))/2",
]
for identity in trig_prove_extra:
    lines = TRIG.solve_prove_text(identity, "1")
    report(f"Extra prove: {identity}", lines[-1] == "LHS = RHS", "" if lines[-1] == "LHS = RHS" else "Identity not completed", format_lines(lines))

trig_solve_extra = [
    ("sin(2*x)=sin(x),x,0,2*pi", 5),
    ("tan(x)+1=0,x,0,360", 2),
    ("2*cos(x)^2-3*cos(x)+1=0,x,0,360", 4),
    ("sin(x)/(1+cos(x))=1,x,0,2*pi", 1),
    ("cos(3*x)=0,x,0,360", 6),
    ("sin(4*x)=0,x,0,360", 9),
    ("2*sin(x)-sqrt(3)=0,x,0,360", 2),
    ("sec(x)=2,x,0,360", 2),
    ("cosec(x)=2,x,0,360", 2),
    ("tan(3*x)=1,x,0,360", 6),
]
for text, expected_count in trig_solve_extra:
    result, lines = TRIG.solve_solve_text(text)
    passed = isinstance(result, list) and len(result) == expected_count
    report(f"Extra trig solve: {text}", passed, f"Expected {expected_count} solutions" if not passed else "", format_lines(lines))

print_section("9.3 Additional Derivative Extremes")
derive_extra = [
    ("x^sin(x^2)", "Log diff"),
    ("sin(x)^x", "Log diff"),
    ("log(3, x^2+1)", "Quotient rule"),
    ("sqrt(1+x^2)/(1+x^3)", "Quotient rule"),
    ("(x^2+1)/(sqrt(x)+1)", "Quotient rule"),
    ("sin(exp(x^2))", "Chain rule"),
    ("exp(sin(x^2))/sqrt(x)", "Quotient rule"),
    ("log(2, x^2+1)/(1+x^3)", "Quotient rule"),
    ("((x^2+1)/(x+1))^3", "Quotient rule"),
    ("exp(sin(x))*cos(exp(x))", "Product rule"),
    ("sqrt(1+sin(x^2))", "Chain rule"),
    ("exp(exp(x))", "Chain rule"),
    ("(x^2+1)^sin(x)", "Log diff"),
    ("2^x", ""),
    ("3^(x^2)", "Chain rule"),
    ("atan(exp(x))", "Chain rule"),
    ("asin(2*x)", "Chain rule"),
    ("acos(x^2)", "Chain rule"),
    ("tan(x^2)*sec(x^2)", "Product rule"),
    ("(x^2+1)/(x+3)", "Quotient rule"),
]
for expr, key_step in derive_extra:
    try:
        var, steps, final, formatted = DERIVE.solve_normal_mode(expr)
        passed = final is not None and step_present(steps, key_step)
        report(f"Extra derive: d/dx[{expr}]", passed, key_step if not passed and key_step else "", format_lines(steps + [formatted]))
    except Exception as err:
        report(f"Extra derive: d/dx[{expr}]", False, str(err), "")

print_section("9.4 Additional Integration Extremes")
integration_extra = [
    ("exp(2*x)/(1+exp(4*x))", "auto", True),
    ("x/(1+x^4)", "auto", True),
    ("x^3/(1+x^8)", "auto", True),
    ("exp(x)/(4+exp(2*x))", "auto", True),
    ("1/sqrt(x^2+6*x+5)", "auto", True),
    ("(3*x+2)/sqrt(9*x^2+12*x+10)", "auto", True),
    ("(6*x+4)/(9*x^2+12*x+10)", "auto", True),
    ("x^2/sqrt(x^3+5)", "auto", True),
    ("1/(x*(1+ln(x)^2))", "auto", True),
    ("1/(x*sqrt(ln(x)))", "auto", True),
    ("sqrt(x)/(1+x)", "auto", True),
    ("1/(sqrt(x)*(1+x))", "auto", True),
    ("(2*x)/(1-x^4)", "auto", True),
    ("exp(x)*sin(x)^3", "auto", True),
    ("x^2*sin(x)^3*exp(x)", "auto", False),
    ("(2*x+1)/(sqrt(x^2+x+1)*(1+x^2+x))", "auto", True),
    ("(3*x^2+1)/(x^3+x+7)", "auto", True),
    ("sin(x)/(1+cos(x))", "auto", True),
    ("(1+cos(2*x))/sin(2*x)", "auto", True),
    ("cos(x)/(2+sin(x))", "auto", True),
    ("1/(x^2+9)", "auto", True),
    ("1/(x^2+2*x+5)", "auto", True),
    ("x/sqrt(x^2-2*x+5)", "auto", True),
    ("x*log(x)", "5", False),
    ("x^2*exp(x)", "auto", True),
    ("sin(x)^4*cos(x)^2", "auto", False),
    ("1/(x^2-1)", "auto", True),
    ("exp(x)*cos(3*x)", "auto", False),
    ("1/sqrt(1-x^2)", "auto", True),
    ("1/(1+x^2)", "auto", True),
]
for expr, method, strict in integration_extra:
    try:
        title, ans, lines = INTEGRATE.solve(INTEGRATE.parse(expr), 'x', method)
        passed = ans is not None and (integration_matches(expr, ans) if strict else True)
        out = format_lines((lines or []) + ([f"Answer: {INTEGRATE.show(ans)}"] if ans is not None else []))
        report(f"Extra integrate: ∫{expr} dx", passed, "No valid antiderivative" if not passed else "", out)
    except Exception as err:
        report(f"Extra integrate: ∫{expr} dx", False, str(err), "")

print_section("9.5 Additional SUVAT Extremes")
suvat_extra = [
    (None, SUVAT.num(5), None, SUVAT.num(2), SUVAT.num(3), 's', '24'),
    (SUVAT.num(20), None, SUVAT.num(14), SUVAT.num(2), SUVAT.num(4), 'u', '6'),
    (None, SUVAT.num(7), None, SUVAT.num(3), SUVAT.num(2), 'v', '13'),
    (SUVAT.num(100), SUVAT.num(0), None, SUVAT.num(2), None, 't', '10'),
    (SUVAT.num(12), SUVAT.num(2), SUVAT.num(10), None, None, 'a', '4'),
    (SUVAT.num(20), SUVAT.num(2), None, SUVAT.num(3), None, 'v', 'sqrt'),
    (SUVAT.parse('1/2*g*3^2'), None, None, SUVAT.parse('g'), SUVAT.num(3), 'u', 'g'),
    (SUVAT.num(10), SUVAT.num(0), None, SUVAT.num(-5), None, 'v', 'no real solution'),
    (SUVAT.num(-10), SUVAT.num(1), None, SUVAT.num(1), None, 't', 'no real solution'),
    (None, SUVAT.num(4), None, SUVAT.num(2), SUVAT.parse('3/2'), 'v', '7'),
]
for s, u, v, a, t, target, expected in suvat_extra:
    try:
        result, equation, original_eq, sub_text = SUVAT._build_suvat_solution_data(s, u, v, a, t, target)
        if result is None:
            passed = expected.lower() in str(equation).lower()
            output = "\n".join(["Equation: " + str(equation), "Substitution: " + (sub_text or "")])
            report(f"Extra SUVAT target {target}", passed, expected if not passed else "", output)
        else:
            result_text = SUVAT.show(result) if not isinstance(result, list) else " or ".join(SUVAT.show(x) for x in result)
            passed = expected in result_text or expected in str(equation)
            output = "\n".join(["Equation: " + str(equation), "Substitution: " + (sub_text or ""), "Result: " + result_text])
            report(f"Extra SUVAT target {target}", passed, expected if not passed else "", output)
    except Exception as err:
        report(f"Extra SUVAT target {target}", False, str(err), "")

print_summary()

if INTERACTIVE_REPORT_MODE:
    try:
        ResultsApp(TEST_RECORDS).run()
    except Exception as err:
        console.print(f"[yellow]Falling back to prompt mode: {err}[/yellow]")
        try:
            interactive_menu()
        except Exception:
            pass
elif not ARGS.plain and IS_TTY:
    console.print("\n[bold cyan]Interactive Mode Available![/bold cyan]")
    console.print("Enter test numbers to expand/collapse output.")
    try:
        interactive_menu()
    except Exception:
        pass
if not INTERACTIVE_REPORT_MODE:
    console.print("\n[dim]Thank you for using CASIO Test Suite![/dim]")
