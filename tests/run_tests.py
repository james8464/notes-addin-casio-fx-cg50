#!/usr/bin/env python3
"""
CASIO Test Suite - Interactive TUI
Expandable test results with rich interface.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from fractions import Fraction
import os
import random
import re
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass
from enum import Enum

try:
    from textual.app import App, ComposeResult
    from textual.containers import Container, Vertical, Horizontal
    from textual.widgets import Static, Input, RichLog
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

class TestStatus(Enum):
    PASS = "pass"
    FAIL = "fail"

@dataclass
class TestRecord:
    test_id: int
    label: str
    status: TestStatus
    output: str
    program: str


_DEFAULT_FORBIDDEN_SNIPPETS = (
    "err:",
    "traceback",
    "division by zero",
    "needs poly support",
    "unsupported inverse family",
    "bad mode",
)


def _default_case_workers():
    override = os.environ.get("CASIO_TEST_WORKERS")
    if override:
        try:
            return max(1, int(override))
        except ValueError:
            pass
    return max(2, min(4, os.cpu_count() or 4))


CASE_WORKERS = _default_case_workers()


def normalized_text(text):
    return " ".join((text or "").lower().split())


def numbered_step_count(text):
    return sum(1 for line in (text or "").splitlines() if re.match(r"^\s*\d+\.", line))


def nonempty_line_count(text):
    return sum(1 for line in (text or "").splitlines() if line.strip())


def build_checker(*, contains_all=(), contains_any=(), min_steps=0, min_lines=0, forbid=()):
    required_all = tuple(item.lower() for item in contains_all if item)
    required_any = tuple(item.lower() for item in contains_any if item)
    forbidden = tuple(item.lower() for item in (_DEFAULT_FORBIDDEN_SNIPPETS + tuple(forbid)))

    def check(output):
        compact = normalized_text(output)
        if any(item in compact for item in forbidden):
            return False
        if min_steps and numbered_step_count(output) < min_steps:
            return False
        if min_lines and nonempty_line_count(output) < min_lines:
            return False
        if required_all and not all(item in compact for item in required_all):
            return False
        if required_any and not any(item in compact for item in required_any):
            return False
        return True

    return check


def algebra_compare_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("equivalent", "result:"),
        contains_any=("simplify",),
        min_steps=8,
        min_lines=8,
    )


def algebra_transform_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("final =",),
        contains_any=("use ", "rewrite", "match coefficients", "factor out"),
        min_steps=4,
        min_lines=4,
    )


def algebra_solve_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("expr =", "x ="),
        contains_any=("solve",),
        min_steps=2,
        min_lines=3,
    )


def algebra_inverse_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("f^-1", "y ="),
        min_steps=4,
        min_lines=4,
    )


def algebra_comp_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("f(x) =", "g(x) =", "f(g(x))"),
        min_steps=4,
        min_lines=5,
    )


def algebra_rewrite_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("final =",),
        contains_any=("write in terms of", "rewrite"),
        min_steps=4,
        min_lines=4,
    )


def algebra_expand_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("out =",),
        contains_any=("binomial expansion", "expand"),
        min_steps=5,
        min_lines=5,
    )


def algebra_complete_square_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("ans =",),
        contains_any=("complete square",),
        min_steps=3,
        min_lines=4,
    )


def trig_prove_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("lhs = rhs",),
        contains_any=("use ", "= rhs", "= lhs", "hence"),
        min_steps=4,
        min_lines=4,
    )


def trig_transform_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("use ", "hence", "= lhs"),
        min_steps=3,
        min_lines=4,
    )


def trig_solve_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("solve trig eq", "x ="),
        contains_any=("start with",),
        min_steps=6,
        min_lines=6,
    )


def trig_rewrite_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("final =",),
        contains_any=("write using", "use "),
        min_steps=4,
        min_lines=4,
    )


def derive_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("dy/dx",),
        contains_any=("chain rule", "product rule", "quotient rule", "log diff", "implicit"),
        min_steps=2,
        min_lines=4,
    )


def derive_implicit_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("dy/dx",),
        contains_any=("d/dx(lhs)=d/dx(rhs)", "make dy/dx"),
        min_steps=3,
        min_lines=5,
    )


def derive_param_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("dx/dt", "dy/dt", "dy/dx ="),
        min_steps=0,
        min_lines=3,
    )


def integrate_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("+ c",),
        contains_any=(
            "method:",
            "u =",
            "use the standard result",
            "integration by parts",
            "split the numerator",
            "complete the square",
            "partial fractions",
        ),
        min_steps=2,
        min_lines=3,
    )


def format_suvat_working(result, equation, original_eq, substitution):
    lines = []
    if original_eq:
        lines.append(f"Original equation: {original_eq}")
    if equation and str(equation) != str(original_eq):
        lines.append(f"Rearranged equation: {equation}")
    elif equation:
        lines.append(f"Equation: {equation}")
    if substitution:
        lines.append(f"Substitute: {substitution}")
    if result is not None:
        lines.append(f"Answer: {SUVAT.show(result)}")
    return "\n".join(lines)


def suvat_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("equation:", "substitute:", "answer:"),
        min_lines=3,
        forbid=("error",),
    )

class CASIOApp(App):
    CSS = """
    App {
        background: #242934;
    }

    Screen {
        background: #242934;
        color: #e5e7eb;
    }

    #shell {
        width: 100%;
        height: 100%;
        padding: 0 1;
        background: #242934;
    }

    #chrome {
        height: 1;
        padding: 0 0 0 1;
        color: #a8adb8;
        background: #242934;
    }

    #chrome-title {
        width: 1fr;
        content-align: left middle;
        color: #a8adb8;
    }

    #hero {
        height: 8;
        margin: 1 0 1 0;
        padding: 0;
        border: round #d57a56;
        background: #2b3140;
    }

    #hero-title {
        height: 1;
        padding: 0 1 0 2;
        color: #d57a56;
        background: #2b3140;
    }

    #hero-body {
        height: 1fr;
        background: #2b3140;
    }

    #hero-left {
        width: 28;
        min-width: 28;
        padding: 1 2;
        border-right: solid #d57a56 35%;
        background: #2b3140;
        content-align: center middle;
    }

    #hero-right {
        width: 1fr;
        padding: 1 2;
        background: #2b3140;
    }

    #hero-copy {
        color: #d9dce3;
    }

    #divider {
        height: 1;
        margin: 0 0 1 0;
        color: #7b8190;
        background: #242934;
    }

    #results-pane {
        width: 100%;
        height: 1fr;
        min-height: 1;
        margin: 0;
        padding: 0 1;
        background: #242934;
    }

    #results {
        width: 100%;
        height: 1fr;
        min-height: 1;
        padding: 0;
        background: #242934;
        border: none;
        scrollbar-size: 0 0;
    }

    #results .rich-log--line {
        background: #242934;
    }

    #prompt-rule-top {
        height: 1;
        color: #9a9a98;
        background: #242934;
    }

    #prompt-row {
        height: 2;
        padding: 0 1;
        background: #242934;
    }

    #prompt-glyph {
        width: 3;
        color: #e5e7eb;
        content-align: left middle;
    }

    #command-suggestions {
        height: auto;
        min-height: 0;
        max-height: 8;
        padding: 0 1 0 4;
        color: #a7acb8;
        background: #242934;
    }

    #prompt-rule-bottom {
        height: 1;
        margin: 0 0 1 0;
        color: #9a9a98;
        background: #242934;
    }

    #command-line {
        height: 1;
        padding: 0;
        background: #242934;
    }

    #command-input {
        width: 1fr;
        border: none;
        background: #242934;
        color: #e5e7eb;
        padding: 0;
    }

    #command-input > .input--cursor {
        background: #e5e7eb;
        color: #242934;
    }

    #command-help {
        height: 1;
        padding: 0 1 0 4;
        color: #a7acb8;
        background: #242934;
    }

    #status-line {
        height: 1;
        padding: 0 1;
        color: #b9bdc7;
        background: #242934;
    }

    Input {
        background: #242934;
    }

    RichLog {
        background: #242934;
    }

    Container {
        background: #242934;
    }

    Vertical {
        background: #242934;
    }

    Static {
        background: #242934;
    }

    .scrollbar {
        display: none;
    }
    """

    BINDINGS = [
        ("r", "run_tests", "Run tests"),
        ("c", "clear", "Clear"),
        ("slash", "focus_command", "Command"),
        ("end", "scroll_to_bottom", "Bottom"),
        ("ctrl+c", "quit", "Quit"),
    ]

    def __init__(self):
        super().__init__()
        self.records = []
        self.running = False
        self.current_program = "all"
        self.current_difficulty = "all"
        self.last_run_scope = ("all", "all")
        self.command_items = {
            "/run": "Run all tests in the current scope",
            "/run all": "Run every test across every program",
            "/run easy": "Run easy tests for every program",
            "/run medium": "Run medium tests for every program",
            "/run hard": "Run hard tests for every program",
            "/run algebra easy": "Run easy Algebra tests",
            "/run algebra medium": "Run medium Algebra tests",
            "/run algebra hard": "Run hard Algebra tests",
            "/run trigonometry easy": "Run easy Trigonometry tests",
            "/run trigonometry medium": "Run medium Trigonometry tests",
            "/run trigonometry hard": "Run hard Trigonometry tests",
            "/run derive easy": "Run easy Derive tests",
            "/run derive medium": "Run medium Derive tests",
            "/run derive hard": "Run hard Derive tests",
            "/run integrate easy": "Run easy Integrate tests",
            "/run integrate medium": "Run medium Integrate tests",
            "/run integrate hard": "Run hard Integrate tests",
            "/run suvat easy": "Run easy SUVAT tests",
            "/run suvat medium": "Run medium SUVAT tests",
            "/run suvat hard": "Run hard SUVAT tests",
            "/rerun": "Run the previous test scope again",
            "/clear": "Reset the output view",
            "/quit": "Exit the test harness",
            "/status": "Show the current scope and last run summary",
            "/report": "Show detailed failures from the last run",
            "/fails": "List failed tests from the last run",
            "/programs": "List available test programs",
            "/help": "Show useful commands",
            "/filter all": "Show every program",
            "/filter algebra": "Only run Algebra tests",
            "/filter trigonometry": "Only run Trigonometry tests",
            "/filter derive": "Only run Derive tests",
            "/filter integrate": "Only run Integrate tests",
            "/filter suvat": "Only run SUVAT tests",
        }

    def compose(self):
        with Container(id="shell"):
            with Container(id="chrome"):
                yield Static("CASIO Test Suite v1.0", id="chrome-title")

            with Container(id="hero"):
                yield Static("[bold #e7895f]CASIO Test Suite[/bold #e7895f] [dim]v1.0[/dim]", id="hero-title")
                with Horizontal(id="hero-body"):
                    with Container(id="hero-left"):
                        yield Static(
                            "[bold]Welcome back![/bold]\n\n"
                            "   .-.\n"
                            "  (o o)\n"
                            "  | O \\\n"
                            "   \\   \\\n"
                            "    `~~~'\n\n"
                            "[dim]gpt-5.4 · API Usage Billing[/dim]\n"
                            "[dim]~/Developer/Python/CASIO[/dim]",
                        )
                    with Container(id="hero-right"):
                        yield Static(
                            "[bold #a66552]Tips for getting started[/bold #a66552]\n"
                            "Run [bold]/run[/bold] to execute the CASIO test suite.\n\n"
                            "[bold #a66552]Recent activity[/bold #a66552]\n"
                            "[dim]Now[/dim] Ready to run algebra, trig, derive, integrate, and SUVAT checks.\n"
                            "[dim]Hint[/dim] Type [bold]/[/bold] to browse commands.\n"
                            "[dim]Resume[/dim] Use [bold]/report[/bold] or [bold]/fails[/bold] after a run for investigation.",
                            id="hero-copy",
                        )

            yield Static("", id="divider")

            with Vertical(id="results-pane"):
                yield RichLog(id="results", wrap=False, markup=True, auto_scroll=True, highlight=False, max_lines=1000)

            yield Static("", id="prompt-rule-top")
            with Horizontal(id="prompt-row"):
                yield Static("›", id="prompt-glyph")
                with Container(id="command-line"):
                    yield Input(placeholder="", id="command-input")
            yield Static("", id="prompt-rule-bottom")
            yield Static("", id="command-help")
            yield Static("", id="command-suggestions")
            yield Static("Ready", id="status-line")

    def on_mount(self):
        self.title = "CASIO Test Suite"
        self.sub_title = "Interactive maths harness"
        self.refresh_rule_lines()
        self.query_one("#command-input", Input).focus()
        self.action_clear()

    def on_resize(self, event) -> None:
        self.refresh_rule_lines()

    def refresh_rule_lines(self):
        line_width = max(self.size.width - 4, 20)
        line = "─" * line_width
        for selector in ("#divider", "#prompt-rule-top", "#prompt-rule-bottom"):
            self.query_one(selector, Static).update(line)

    def action_focus_command(self):
        input_widget = self.query_one("#command-input", Input)
        if not input_widget.value.startswith("/"):
            input_widget.value = "/"
        input_widget.cursor_position = len(input_widget.value)
        input_widget.focus()
        self.update_command_help(input_widget.value)

    def action_scroll_to_bottom(self):
        self.query_one("#results", RichLog).scroll_end(animate=False)
        self.query_one("#command-input", Input).focus()

    def normalize_program_name(self, value: str):
        mapping = {
            "all": "all",
            "algebra": "Algebra",
            "trigonometry": "Trigonometry",
            "trig": "Trigonometry",
            "derive": "Derive",
            "integration": "Integrate",
            "integrate": "Integrate",
            "int": "Integrate",
            "suvat": "SUVAT",
        }
        return mapping.get(value.lower())

    def normalize_difficulty(self, value: str):
        mapping = {
            "all": "all",
            "easy": "easy",
            "medium": "medium",
            "hard": "hard",
        }
        return mapping.get(value.lower())

    def parse_run_scope(self, value: str):
        parts = value.split()
        if not parts or parts[0] != "/run":
            return None
        if len(parts) == 1:
            return self.current_program, self.current_difficulty
        if len(parts) == 2 and parts[1].lower() == "all":
            return "all", "all"

        program = self.current_program
        difficulty = self.current_difficulty

        for token in parts[1:]:
            maybe_program = self.normalize_program_name(token)
            maybe_difficulty = self.normalize_difficulty(token)
            if maybe_program is not None:
                program = maybe_program
                continue
            if maybe_difficulty is not None:
                difficulty = maybe_difficulty
                continue
            return None
        return program, difficulty

    def on_key(self, event) -> None:
        if event.key == "ctrl+p":
            event.prevent_default()
            event.stop()

    def on_input_changed(self, event: Input.Changed) -> None:
        self.update_command_help(event.value)

    def on_input_submitted(self, event: Input.Submitted) -> None:
        value = event.value.strip()
        event.input.value = ""
        self.update_command_help("")
        self.query_one("#command-suggestions", Static).update("")

        if not value:
            return

        run_scope = self.parse_run_scope(value)
        if run_scope is not None:
            program, difficulty = run_scope
            self.current_program = program
            self.current_difficulty = difficulty
            self.action_run_tests(command_label=value)
        elif value == "/rerun":
            self.current_program, self.current_difficulty = self.last_run_scope
            self.append_result(
                f"[bold #e07a53]Re-running:[/bold #e07a53] "
                f"{self.current_program} · {self.current_difficulty}"
            )
            self.action_run_tests(command_label="/rerun")
        elif value == "/clear":
            self.action_clear()
        elif value == "/quit":
            self.action_quit()
        elif value == "/status":
            self.show_status()
        elif value == "/report":
            self.show_report()
        elif value == "/fails":
            self.show_fails()
        elif value == "/programs":
            self.show_programs()
        elif value == "/help":
            self.show_help()
        elif value.startswith("/filter "):
            raw = value.split(" ", 1)[1].strip().lower()
            selected = self.normalize_program_name(raw)
            if selected is None:
                self.append_result(f"[bold #f59e0b]Unknown filter:[/bold #f59e0b] {raw}")
                self.update_summary("Unknown filter")
            else:
                self.current_program = selected
                self.append_result(f"[bold #e07a53]Filter set:[/bold #e07a53] {selected}")
                self.update_summary(f"Selected: {selected} · {self.current_difficulty}")
        else:
            self.append_result(f"[bold #f59e0b]Unknown command:[/bold #f59e0b] {value}")
            self.update_summary("Unknown command")

    def update_command_help(self, value: str):
        help_widget = self.query_one("#command-help", Static)
        suggestions_widget = self.query_one("#command-suggestions", Static)

        if not value:
            help_widget.update("? for shortcuts")
            suggestions_widget.update("")
            return

        if not value.startswith("/"):
            help_widget.update("Commands start with /")
            suggestions_widget.update("")
            return

        matches = [(cmd, desc) for cmd, desc in self.command_items.items() if cmd.startswith(value)]
        if matches:
            help_widget.update(f"{len(matches)} command match{'es' if len(matches) != 1 else ''}")
            width = max(len(cmd) for cmd, _ in matches[:6]) + 2
            lines = []
            for index, (cmd, desc) in enumerate(matches[:6]):
                if index == 0:
                    lines.append(
                        f"[bold #e07a53]{cmd.ljust(width)}[/bold #e07a53]"
                        f"[#cfd3dc]{desc}[/#cfd3dc]"
                    )
                else:
                    lines.append(
                        f"[#8f95a3]{cmd.ljust(width)}[/#8f95a3]"
                        f"[#b7bcc7]{desc}[/#b7bcc7]"
                    )
            suggestions_widget.update("\n".join(lines))
        else:
            help_widget.update("No matching commands")
            suggestions_widget.update("")

    def show_help(self):
        self.append_result("[bold #e07a53]Available commands[/bold #e07a53]")
        self.append_result("")
        width = max(len(cmd) for cmd in self.command_items) + 2
        for cmd, desc in self.command_items.items():
            self.append_result(f"[bold #e07a53]{cmd.ljust(width)}[/bold #e07a53][dim]{desc}[/dim]")
        self.append_result("")
        self.append_result("[dim]Tip: type / to browse matching commands interactively.[/dim]")
        self.update_summary("Help")

    def show_programs(self):
        self.append_result("[bold #e07a53]Programs[/bold #e07a53]")
        self.append_result("[dim]Algebra[/dim] — symbolic comparison, transforms, solving, inverses")
        self.append_result("[dim]Trigonometry[/dim] — identities, transforms, equation solving")
        self.append_result("[dim]Derive[/dim] — differentiation and harder chain-rule cases")
        self.append_result("[dim]Integrate[/dim] — standard integrals, substitution, parts, extremes")
        self.append_result("[dim]SUVAT[/dim] — motion equations and projectile-style checks")
        self.update_summary("Programs")

    def show_status(self):
        total = len(self.records)
        passed = sum(1 for r in self.records if r.status == TestStatus.PASS)
        failed = total - passed
        self.append_result("[bold #e07a53]Status[/bold #e07a53]")
        self.append_result(f"[dim]Current program:[/dim] {self.current_program}")
        self.append_result(f"[dim]Current difficulty:[/dim] {self.current_difficulty}")
        self.append_result(f"[dim]Last run scope:[/dim] {self.last_run_scope[0]} · {self.last_run_scope[1]}")
        self.append_result(f"[dim]Last run totals:[/dim] {passed} passed, {failed} failed, {total} total")
        self.update_summary(f"Status · {passed}/{total} passed" if total else "Status · no runs yet")

    def show_fails(self):
        failed = [r for r in self.records if r.status == TestStatus.FAIL]
        if not self.records:
            self.append_result("[bold #f59e0b]No run data available yet.[/bold #f59e0b]")
            self.update_summary("No runs yet")
            return
        if not failed:
            self.append_result("[bold #22c55e]No failed tests in the last run.[/bold #22c55e]")
            self.update_summary("No failures")
            return

        self.append_result("[bold #e07a53]Failed tests[/bold #e07a53]")
        for record in failed:
            self.append_result(f"[#f87171]✕[/#f87171] [{record.program}] {record.label}")
        self.update_summary(f"{len(failed)} failures")

    def show_report(self):
        failed = [r for r in self.records if r.status == TestStatus.FAIL]
        if not self.records:
            self.append_result("[bold #f59e0b]No run data available yet.[/bold #f59e0b]")
            self.update_summary("No runs yet")
            return
        if not failed:
            self.append_result("[bold #22c55e]All tests passed in the last run.[/bold #22c55e]")
            self.update_summary("All tests passed")
            return

        self.append_result("[bold #e07a53]Failure report[/bold #e07a53]")
        for index, record in enumerate(failed, start=1):
            self.append_result(f"[bold #f87171]{index}. [{record.program}] {record.label}[/bold #f87171]")
            output = (record.output or "").strip()
            if not output:
                self.append_result("[dim]No captured output.[/dim]")
            else:
                lines = output.splitlines()
                for line in lines[:12]:
                    self.append_result(f"[dim]    {line}[/dim]")
                if len(lines) > 12:
                    self.append_result("[dim]    ... output truncated ...[/dim]")
            self.append_result("")
        self.update_summary(f"Report · {len(failed)} failures")

    def action_run_tests(self, command_label="/run"):
        if self.running:
            return
        self.running = True
        self.records.clear()
        self.last_run_scope = (self.current_program, self.current_difficulty)
        self.query_one("#results", RichLog).clear()
        self.append_result(f"[bold #e07a53]▶ {command_label}[/bold #e07a53]")
        self.append_result("[dim]Booting test harness...[/dim]")
        self.update_summary(f"Running {self.current_program} · {self.current_difficulty}...")
        self.run_all_tests()

    def action_clear(self):
        self.records.clear()
        log = self.query_one("#results", RichLog)
        log.clear()
        log.write("")
        self.query_one("#command-suggestions", Static).update("")
        self.query_one("#command-help", Static).update("? for shortcuts")
        self.update_summary("Ready")

    def action_quit(self):
        self.exit()

    def run_all_tests(self):
        def run():
            self.call_from_thread(
                self.append_result,
                "[bold #e07a53]▶ Running tests...[/bold #e07a53]",
            )
            self.call_from_thread(
                self.append_result,
                f"[dim]Streaming live results from {self.current_program} · {self.current_difficulty}[/dim]",
            )

            programs = [
                ("Algebra", self.run_algebra),
                ("Trigonometry", self.run_trig),
                ("Derive", self.run_derive),
                ("Integrate", self.run_integrate),
                ("SUVAT", self.run_suvat),
            ]

            if self.current_program != "all":
                programs = [(name, func) for name, func in programs if name == self.current_program]

            for name, func in programs:
                self.call_from_thread(self.append_result, "")
                self.call_from_thread(
                    self.append_result,
                    f"[bold #e07a53]▶ {name}[/bold #e07a53]",
                )
                self.call_from_thread(
                    self.append_result,
                    "[dim]────────────────────────────────────────[/dim]",
                )
                func(self.current_difficulty)
            self.running = False
            self.call_from_thread(self.render_summary)

        import threading
        threading.Thread(target=run, daemon=True).start()

    def append_result(self, text):
        try:
            self.query_one("#results", RichLog).write(text)
        except Exception:
            pass

    def add_test(self, label, passed, output, program):
        icon = "●" if passed else "✕"
        color = "#22c55e" if passed else "#f87171"

        status = TestStatus.PASS if passed else TestStatus.FAIL
        self.records.append(TestRecord(len(self.records)+1, label, status, output, program))

        self.append_result(f"[{color}]{icon}[/{color}] {label}")

        passed_count = sum(1 for r in self.records if r.status == TestStatus.PASS)
        total = len(self.records)
        pct = (passed_count / total * 100) if total > 0 else 0
        self.update_summary(f"Running... {passed_count}/{total} passed ({pct:.0f}%)")

    def render_summary(self):
        passed = sum(1 for r in self.records if r.status == TestStatus.PASS)
        total = len(self.records)
        pct = (passed / total * 100) if total > 0 else 0

        if pct == 100:
            msg = f"● All tests passed · {passed}/{total}"
            color = "#22c55e"
        else:
            msg = f"● {passed}/{total} passed ({pct:.0f}%)"
            color = "#fbbf24"

        self.append_result(f"\n[bold]Results:[/bold] [bold {color}]{msg}[/bold {color}]\n")
        self.update_summary(msg)

    def update_summary(self, text):
        try:
            self.query_one("#status-line", Static).update(text)
        except Exception:
            pass

    def run_cli(self, script, inp):
        proc = subprocess.run([PY, str(ROOT/script)], input=inp, text=True, capture_output=True)
        return proc.stdout, proc.stderr

    def run_simple_cases(self, script, program, cases, default_check):
        def evaluate(case):
            inp, label, expected = case
            out, _ = self.run_cli(script, inp)
            if callable(expected):
                passed = expected(out)
            elif default_check == "contains":
                passed = expected in out
            elif default_check == "no_error":
                passed = "Error" not in out and expected in out
            else:
                passed = expected in out
            return label, passed, out

        with ThreadPoolExecutor(max_workers=CASE_WORKERS) as executor:
            for label, passed, out in executor.map(evaluate, cases):
                self.add_test(label, passed, out, program)

    def run_algebra(self, difficulty="all"):
        p = "Algebra"

        # Core comparison tests
        tests = [
            ("1\n(x+1)^2\nx^2+2*x+1\n", "Equivalent", "Compare: (x+1)^2 ≟ x^2+2x+1"),
            ("1\nsin(x)^2+cos(x)^2\n1\n", "Equivalent", "Compare: sin²x+cos²x ≟ 1"),
            ("1\n(1-cos(2*x))/2\nsin(x)^2\n", "Equivalent", "Compare: (1-cos2x)/2 ≟ sin²x"),
            ("1\n(x^2-1)/(x-1)\nx+1\n", "Equivalent", "Compare: (x²-1)/(x-1) ≟ x+1"),
            ("1\nexp(log(x))\nx\n", "Equivalent", "Compare: e^ln(x) ≟ x"),
            ("1\nlog(e^x)\nx\n", "Equivalent", "Compare: ln(e^x) ≟ x"),
            ("1\n(x+2)*(x+3)\nx^2+5*x+6\n", "Equivalent", "Compare: (x+2)(x+3) ≟ x²+5x+6"),
            ("1\n(sec(x))^2-(tan(x))^2\n1\n", "Equivalent", "Compare: sec²x-tan²x ≟ 1"),
            ("1\n(cosec(x))^2-(cot(x))^2\n1\n", "Equivalent", "Compare: csc²x-cot²x ≟ 1"),
            ("1\n(x-1)*(x+1)\nx^2-1\n", "Equivalent", "Compare: (x-1)(x+1) ≟ x²-1"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in tests:
                out, _ = self.run_cli("algebraProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Transform tests
        transforms = [
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "2*(sin(x))^2", "Transform: 1-cos(2x) → 2sin²x"),
            ("2\nsin(x)/cos(x)\ntan(x)\n", "tan", "Transform: sinx/cosx → tanx"),
            ("2\n1/cos(x)\nsec(x)\n", "sec", "Transform: 1/cosx → secx"),
            ("2\n1/sin(x)\ncosec(x)\n", "cosec", "Transform: 1/sinx → cscx"),
            ("2\ncos(x)/sin(x)\ncot(x)\n", "cot", "Transform: cosx/sinx → cotx"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "2*sin", "Transform: sin(2x) → 2sinxcosx"),
            ("2\ncos(2*x)\ncos(x)^2-sin(x)^2\n", "cos", "Transform: cos(2x) → cos²x-sin²x"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in transforms:
                out, _ = self.run_cli("algebraProgram.py", inp)
                self.add_test(label, check in out, out, p)

        expand_cases = [
            ("3\n(2*x+1)^5\n\n", "Expand: (2x+1)^5", algebra_expand_checker("32*x^5", "10*x", "1")),
            ("3\n(3*x-2)^6\n\n", "Expand: (3x-2)^6", algebra_expand_checker("729*x^6", "64")),
        ]

        complete_square_cases = [
            ("5\nx^2+6*x+13\n", "Complete square: x^2+6x+13", algebra_complete_square_checker("(x+3)^2+4")),
            ("5\n4*x^2-12*x+25\n", "Complete square: 4x^2-12x+25", algebra_complete_square_checker("(x-3/2)^2", "+16")),
        ]

        if difficulty in ("all", "medium", "hard"):
            self.run_simple_cases("algebraProgram.py", p, expand_cases, "contains")
            self.run_simple_cases("algebraProgram.py", p, complete_square_cases, "contains")

        # Solve tests
        solves = [
            ("6\nx^2+5*x+6=0\n", "x =", "Solve: x²+5x+6=0"),
            ("6\n(x+1)/(x-2)=3\n", "7/2", "Solve: (x+1)/(x-2)=3"),
            ("6\n2*x+6=0\n", "x = -3", "Solve: 2x+6=0"),
            ("6\nx^2-9=0\n", "x =", "Solve: x²-9=0"),
            ("6\n2*x^2+7*x+3=0\n", "x =", "Solve: 2x²+7x+3=0"),
            ("6\n4*x^2-9=0\n", "x =", "Solve: 4x²-9=0"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in solves:
                out, _ = self.run_cli("algebraProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Inverse tests
        inverses = [
            ("8\n(2*x+1)/(3*x+4)\n", "f^-1", "Inverse: (2x+1)/(3x+4)"),
            ("8\n3*x-7\n", "f^-1", "Inverse: 3x-7"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in inverses:
                out, _ = self.run_cli("algebraProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Extreme tests
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("algebraProgram.py", "4\n(x^3-y^3)/((x-y)*(x^2+xy+y^2))\n")
            self.add_test("Extreme: complex fraction", "Error" not in out, out, p)

            out, _ = self.run_cli("algebraProgram.py", "6\nx^5-5*x^4+10*x^3-10*x^2+5*x-1=0\n")
            self.add_test("Extreme: 5th degree polynomial", "x =" in out, out, p)

            out, _ = self.run_cli("algebraProgram.py", "7\n1/(x-1)\nsqrt(x+1)\nx^3+2*x\n")
            self.add_test("Extreme: composition f(g(h(x)))", "Error" not in out, out, p)

        extra_cases = []

        for n in range(2, 12):
            extra_cases.append((
                f"1\n(x+{n})^2\nx^2+{2*n}*x+{n*n}\n",
                f"Generated compare: (x+{n})^2 expansion",
                "Equivalent",
            ))

        for a in range(1, 6):
            for b in range(2, 7):
                extra_cases.append((
                    f"1\n(x+{a})*(x+{b})\nx^2+{a+b}*x+{a*b}\n",
                    f"Generated compare: (x+{a})(x+{b}) expansion",
                    "Equivalent",
                ))

        for n in range(2, 12):
            extra_cases.append((
                f"1\n(x^{n}-1)/(x-1)\n" + "+".join(f"x^{k}" if k > 1 else ("x" if k == 1 else "1") for k in range(n - 1, -1, -1)) + "\n",
                f"Generated compare: geometric factor n={n}",
                "Equivalent",
            ))

        for n in range(1, 11):
            extra_cases.append((
                f"1\nexp(log(x+{n}))\nx+{n}\n",
                f"Generated compare: exp(log(x+{n}))",
                "Equivalent",
            ))

        transform_cases = [
            ("2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n", "Generated transform: sin(3x)", "sin"),
            ("2\ncos(3*x)\n4*cos(x)^3-3*cos(x)\n", "Generated transform: cos(3x)", "cos"),
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "Generated transform: 1-cos(2x)", "2*(sin(x))^2"),
            ("2\n1+cos(2*x)\n2*cos(x)^2\n", "Generated transform: 1+cos(2x)", "2*(cos(x))^2"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "Generated transform: sin(2x)", "2*sin"),
            ("2\nsin(x)/cos(x)\ntan(x)\n", "Generated transform: tan form", "tan"),
            ("2\ncos(x)/sin(x)\ncot(x)\n", "Generated transform: cot form", "cot"),
            ("2\n1/cos(x)\nsec(x)\n", "Generated transform: sec form", "sec"),
            ("2\n1/sin(x)\ncosec(x)\n", "Generated transform: cosec form", "cosec"),
        ]
        extra_cases.extend(transform_cases * 3)

        for r1 in range(-5, 5):
            for r2 in range(r1 + 1, 6):
                extra_cases.append((
                    f"6\nx^2-{r1 + r2}*x+{r1*r2}=0\n",
                    f"Generated solve: roots {r1}, {r2}",
                    "x =",
                ))
                if len(extra_cases) >= 120:
                    break
            if len(extra_cases) >= 120:
                break

        for a in range(2, 12):
            extra_cases.append((
                f"8\n{a}*x-{a+3}\n",
                f"Generated inverse: {a}x-{a+3}",
                "f^-1",
            ))

        for a in range(1, 7):
            for b in range(2, 8):
                extra_cases.append((
                    f"8\n({a}*x+{b})/({b+1}*x+{a+2})\n",
                    f"Generated inverse: ({a}x+{b})/({b+1}x+{a+2})",
                    "f^-1",
                ))

        complex_cases = [
            ("1\nexp(log((x+2)^5))\n(x+2)^5\n", "Complex compare: exp(log((x+2)^5))", "Equivalent"),
            ("1\n((x^2-1)/(x-1))^3\n(x+1)^3\n", "Complex compare: ((x^2-1)/(x-1))^3", "Equivalent"),
            ("1\n(sec(2*x)^2-tan(2*x)^2)*(x+3)^4\n(x+3)^4\n", "Complex compare: trig identity times quartic", "Equivalent"),
            ("1\nexp(log(x^4+3*x^2+1))\nx^4+3*x^2+1\n", "Complex compare: exp(log(quartic))", "Equivalent"),
            ("2\nsin(4*x)/(2*cos(2*x))\nsin(2*x)\n", "Complex transform: sin(4x)/(2cos(2x))", "sin"),
            ("2\n(1-cos(4*x))/2\nsin(2*x)^2\n", "Complex transform: half-angle shifted", "sin"),
            ("6\n(x^2-5*x+6)/(x-3)=0\n", "Complex solve: rational linearised", "x ="),
            ("6\nx^4-5*x^2+4=0\n", "Complex solve: biquadratic", "x ="),
            ("8\n(3*x-5)/(2*x+7)\n", "Complex inverse: fractional linear", "f^-1"),
            ("8\n5*(x-2)^3+1\n", "Complex inverse: shifted cubic", "f^-1"),
            ("1\nlog(exp((x+1)^2))\n(x+1)^2\n", "Complex compare: log(exp((x+1)^2))", "Equivalent"),
            ("1\n((x^3-1)/(x-1))-(x^2+x+1)\n0\n", "Complex compare: cubic factor identity", "Equivalent"),
        ]
        extra_cases.extend(complex_cases)

        extreme_cases = []
        rng = random.Random(20260410)

        for a in range(2, 7):
            for b in range(1, 5):
                extreme_cases.append((
                    f"1\nexp(log(({a}*x^2+1)^3*(x-{b})^2))\n({a}*x^2+1)^3*(x-{b})^2\n",
                    f"Extreme compare: exp-log product a={a}, b={b}",
                    algebra_compare_checker(),
                ))
                extreme_cases.append((
                    f"1\n((x^2-{b}^2)/(x-{b}))^2\n(x+{b})^2\n",
                    f"Extreme compare: removable rational square b={b}",
                    algebra_compare_checker(),
                ))

        for a in range(2, 6):
            for b in range(1, 4):
                extreme_cases.append((
                    f"2\n1-cos(2*({a}*x-{b}))\n2*sin({a}*x-{b})^2\n",
                    f"Extreme transform: 1-cos(2({a}x-{b}))",
                    algebra_transform_checker("sin"),
                ))
                extreme_cases.append((
                    f"2\nsin(2*({a}*x+{b}))\n2*sin({a}*x+{b})*cos({a}*x+{b})\n",
                    f"Extreme transform: sin(2({a}x+{b}))",
                    algebra_transform_checker("sin"),
                ))

        quartic_pairs = [(3, 1), (4, 2), (5, 1), (5, 3), (6, 2), (7, 1), (7, 5), (8, 4), (9, 3), (10, 2)]
        for m, n in quartic_pairs:
            avg = (m * m + n * n) // 2
            gap = (m * m - n * n) // 2
            extreme_cases.append((
                f"6\n(x^2-{avg})^2-{gap * gap}=0\n",
                f"Extreme solve: disguised quartic roots ±{m}, ±{n}",
                algebra_solve_checker(str(m), str(-m), str(n), str(-n)),
            ))

        inverse_specs = [
            ("sqrt({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("1/sqrt({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("log(({a}*x+{b})^3)", algebra_inverse_checker("f^-1")),
            ("{base}^({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("({a}*x+{b})/({c}*x+{d})", algebra_inverse_checker("f^-1")),
        ]
        inverse_params = [
            {"a": 2, "b": 1, "base": 2, "c": 3, "d": 5},
            {"a": 3, "b": 2, "base": 3, "c": 4, "d": 7},
            {"a": 4, "b": 1, "base": 5, "c": 5, "d": 2},
            {"a": 5, "b": 3, "base": 7, "c": 2, "d": 9},
            {"a": 6, "b": 5, "base": 11, "c": 7, "d": 4},
        ]
        for params in inverse_params:
            for pattern, checker in inverse_specs:
                expr = pattern.format(**params)
                extreme_cases.append((
                    f"8\n{expr}\n",
                    f"Extreme inverse: {expr}",
                    checker,
                ))

        comp_specs = [
            ("1/(x^2+{a})", "({b}*x-{c})^2+1"),
            ("log(x+{a})", "({b}*x-{c})^3"),
            ("sqrt(x+{a})", "({b}*x+{c})^2"),
            ("({a}*x+1)/({b}*x+3)", "x^2+{c}"),
        ]
        for a, b, c in [(2, 3, 1), (3, 2, 5), (4, 5, 2), (5, 4, 3), (6, 3, 7)]:
            for f_pattern, g_pattern in comp_specs:
                f_expr = f_pattern.format(a=a, b=b, c=c)
                g_expr = g_pattern.format(a=a, b=b, c=c)
                extreme_cases.append((
                    f"7\n{f_expr}\n{g_expr}\n",
                    f"Extreme composition: f={f_expr}, g={g_expr}",
                    algebra_comp_checker(),
                ))

        rewrite_specs = [
            ("(x+1)^6", ["(x+1)^3"]),
            ("(2*x-1)^8", ["(2*x-1)^2"]),
            ("sin(x)^4+cos(x)^4", ["sin(x)^2", "cos(x)^2"]),
            ("(x^2+1)^6", ["(x^2+1)^3"]),
        ]
        for expr, terms in rewrite_specs:
            term_block = "".join(f"{term}\n" for term in terms)
            extreme_cases.append((
                f"9\n{expr}\n{term_block}\n",
                f"Extreme rewrite: {expr}",
                algebra_rewrite_checker(),
            ))

        rng.shuffle(extreme_cases)

        if difficulty in ("all", "hard"):
            self.run_simple_cases("algebraProgram.py", p, extra_cases[:100], "contains")
            self.run_simple_cases("algebraProgram.py", p, extreme_cases[:100], "contains")

    def run_trig(self, difficulty="all"):
        p = "Trigonometry"

        # Core identity proofs
        proofs = [
            ("1\nsin(x)^2+cos(x)^2=1\n1\n", "LHS = RHS", "Prove: sin²x+cos²x=1"),
            ("1\nsec(x)^2-tan(x)^2=1\n1\n", "LHS = RHS", "Prove: sec²x-tan²x=1"),
            ("1\ncosec(x)^2-cot(x)^2=1\n1\n", "LHS = RHS", "Prove: csc²x-cot²x=1"),
            ("1\nsin(2*x)=2*sin(x)*cos(x)\n1\n", "LHS = RHS", "Prove: sin(2x)=2sinxcosx"),
            ("1\ncos(2*x)=cos(x)^2-sin(x)^2\n1\n", "LHS = RHS", "Prove: cos(2x)=cos²x-sin²x"),
            ("1\n1-cos(2*x)=2*sin(x)^2\n1\n", "LHS = RHS", "Prove: 1-cos(2x)=2sin²x"),
            ("1\n1+cos(2*x)=2*cos(x)^2\n1\n", "LHS = RHS", "Prove: 1+cos(2x)=2cos²x"),
            ("1\ntan(x)=sin(x)/cos(x)\n1\n", "LHS = RHS", "Prove: tanx=sinx/cosx"),
            ("1\ncot(x)=cos(x)/sin(x)\n1\n", "LHS = RHS", "Prove: cotx=cosx/sinx"),
            ("1\nsin(3*x)=3*sin(x)-4*sin(x)^3\n1\n", "LHS = RHS", "Prove: sin(3x)=3sinx-4sin³x"),
            ("1\ncos(3*x)=4*cos(x)^3-3*cos(x)\n1\n", "LHS = RHS", "Prove: cos(3x)=4cos³x-3cosx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in proofs:
                out, _ = self.run_cli("trigProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Transform tests
        transforms = [
            ("2\nsin(x)/cos(x)\ntan(x)\n", "tan", "Transform: sinx/cosx → tanx"),
            ("2\n1/cos(x)\nsec(x)\n", "sec", "Transform: 1/cosx → secx"),
            ("2\n1/sin(x)\ncosec(x)\n", "cosec", "Transform: 1/sinx → cscx"),
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "2*sin", "Transform: 1-cos(2x) → 2sin²x"),
            ("2\n1+cos(2*x)\n2*cos(x)^2\n", "2*cos", "Transform: 1+cos(2x) → 2cos²x"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "2*sin", "Transform: sin(2x) → 2sinxcosx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in transforms:
                out, _ = self.run_cli("trigProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Solve equations
        solves = [
            ("3\nsin(x)=0,x,0,360\n", "x =", "Solve: sinx=0, 0-360°"),
            ("3\ncos(x)=0,x,0,360\n", "x =", "Solve: cosx=0, 0-360°"),
            ("3\ntan(x)=1,x,0,360\n", "x =", "Solve: tanx=1, 0-360°"),
            ("3\nsec(x)=2,x,0,360\n", "x =", "Solve: secx=2, 0-360°"),
            ("3\ncosec(x)=2,x,0,360\n", "x =", "Solve: cscx=2, 0-360°"),
            ("3\ncot(x)=1,x,0,360\n", "x =", "Solve: cotx=1, 0-360°"),
            ("3\nsin(x)=1/2,x,0,360\n", "x =", "Solve: sinx=½, 0-360°"),
            ("3\ncos(x)=1/2,x,0,360\n", "x =", "Solve: cosx=½, 0-360°"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in solves:
                out, _ = self.run_cli("trigProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Regression: non-identities should fail cleanly in prove mode
        if difficulty in ("all", "medium", "hard"):
            out, _ = self.run_cli("trigProgram.py", "1\n2*sin(x-30)=5*cos(x)\n1\n")
            self.add_test("Reject non-identity cleanly: 2sin(x-30)=5cos(x)", "not an identity" in out.lower(), out, p)

        # NEW: Additional proofs
            out, _ = self.run_cli("trigProgram.py", "1\ntan(x)+cot(x)=2*cosec(2*x)\n1\n")
            self.add_test("Prove: tanx+cotx=2csc(2x)", "LHS = RHS" in out, out, p)

        # NEW: Solve more complex equations
            out, _ = self.run_cli("trigProgram.py", "3\nsin(x)+sin(2*x)+sin(3*x)=0,x,0,6.283\n")
            self.add_test("Solve: sinx+sin2x+sin3x=0", "x =" in out, out, p)

            out, _ = self.run_cli("trigProgram.py", "3\nsin(x)*cos(x)=1/2,x,0,360\n")
            self.add_test("Solve: sinx·cosx=½, 0-360°", "x =" in out or "45" in out, out, p)

        # Extreme tests
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("trigProgram.py", "1\nsin(6*x)=6*sin(x)*cos(x)^5-20*sin(x)^3*cos(x)^3+16*sin(x)^5*cos(x)\n1\n")
            self.add_test("Extreme: sin(6x) identity", "LHS = RHS" in out or "RHS" in out, out, p)

        extra_cases = []

        proof_templates = [
            ("1\nsin(4*x)=2*sin(2*x)*cos(2*x)\n1\n", "Generated proof: sin(4x)", "LHS = RHS"),
            ("1\ncos(4*x)=cos(2*x)^2-sin(2*x)^2\n1\n", "Generated proof: cos(4x)", "LHS = RHS"),
            ("1\n1-cos(4*x)=2*sin(2*x)^2\n1\n", "Generated proof: 1-cos(4x)", "LHS = RHS"),
            ("1\n1+cos(4*x)=2*cos(2*x)^2\n1\n", "Generated proof: 1+cos(4x)", "LHS = RHS"),
            ("1\nsin(6*x)=2*sin(3*x)*cos(3*x)\n1\n", "Generated proof: sin(6x)", "LHS = RHS"),
            ("1\ncos(6*x)=cos(3*x)^2-sin(3*x)^2\n1\n", "Generated proof: cos(6x)", "LHS = RHS"),
            ("1\ntan(2*x)=sin(2*x)/cos(2*x)\n1\n", "Generated proof: tan(2x)", "LHS = RHS"),
            ("1\nsec(2*x)^2-tan(2*x)^2=1\n1\n", "Generated proof: sec²(2x)-tan²(2x)", "LHS = RHS"),
            ("1\ncosec(2*x)^2-cot(2*x)^2=1\n1\n", "Generated proof: csc²(2x)-cot²(2x)", "LHS = RHS"),
            ("1\nsin(3*x)=3*sin(x)-4*sin(x)^3\n1\n", "Generated proof: sin(3x) repeat", "LHS = RHS"),
            ("1\ncos(3*x)=4*cos(x)^3-3*cos(x)\n1\n", "Generated proof: cos(3x) repeat", "LHS = RHS"),
            ("1\ntan(x)+cot(x)=2*cosec(2*x)\n1\n", "Generated proof: tan+cot", "LHS = RHS"),
        ]
        extra_cases.extend(proof_templates * 4)

        transform_templates = [
            ("2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n", "Generated transform: sin(3x)", "sin"),
            ("2\ncos(3*x)\n4*cos(x)^3-3*cos(x)\n", "Generated transform: cos(3x)", "cos"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "Generated transform: sin(2x)", "2*sin"),
            ("2\n1-cos(4*x)\n2*sin(2*x)^2\n", "Generated transform: 1-cos(4x)", "sin"),
            ("2\n1+cos(4*x)\n2*cos(2*x)^2\n", "Generated transform: 1+cos(4x)", "cos"),
        ]
        extra_cases.extend(transform_templates * 4)

        solve_templates = [
            ("3\nsin(x)=sqrt(3)/2,x,0,360\n", "Generated solve: sinx=√3/2", "x ="),
            ("3\nsin(x)=sqrt(2)/2,x,0,360\n", "Generated solve: sinx=√2/2", "x ="),
            ("3\ncos(x)=sqrt(3)/2,x,0,360\n", "Generated solve: cosx=√3/2", "x ="),
            ("3\ncos(x)=-1/2,x,0,360\n", "Generated solve: cosx=-1/2", "x ="),
            ("3\ntan(x)=sqrt(3),x,0,360\n", "Generated solve: tanx=√3", "x ="),
            ("3\ntan(x)=-1,x,0,360\n", "Generated solve: tanx=-1", "x ="),
            ("3\nsec(x)=-2,x,0,360\n", "Generated solve: secx=-2", "x ="),
            ("3\ncosec(x)=-2,x,0,360\n", "Generated solve: cscx=-2", "x ="),
            ("3\ncot(x)=-1,x,0,360\n", "Generated solve: cotx=-1", "x ="),
            ("3\nsin(2*x)=0,x,0,360\n", "Generated solve: sin(2x)=0", "x ="),
        ]
        extra_cases.extend(solve_templates * 4)

        complex_cases = [
            ("1\nsin(2*x+1)^2+cos(2*x+1)^2=1\n1\n", "Complex proof: shifted Pythagorean identity", "LHS = RHS"),
            ("1\n1-cos(2*(3*x-1))=2*sin(3*x-1)^2\n1\n", "Complex proof: nested double-angle", "LHS = RHS"),
            ("1\ntan(3*x-2)=sin(3*x-2)/cos(3*x-2)\n1\n", "Complex proof: shifted tan identity", "LHS = RHS"),
            ("1\nsec(3*x+1)^2-tan(3*x+1)^2=1\n1\n", "Complex proof: shifted sec identity", "LHS = RHS"),
            ("2\n1-cos(2*(x+1))\n2*sin(x+1)^2\n", "Complex transform: nested double-angle", "sin"),
            ("2\nsin(2*(2*x-1))\n2*sin(2*x-1)*cos(2*x-1)\n", "Complex transform: nested sin double-angle", "2*sin"),
            ("3\nsin(2*x)=sqrt(3)/2,x,0,360\n", "Complex solve: sin(2x)=√3/2", "x ="),
            ("3\ncos(3*x)=1/2,x,0,360\n", "Complex solve: cos(3x)=1/2", "x ="),
            ("3\ntan(2*x)=-1,x,0,360\n", "Complex solve: tan(2x)=-1", "x ="),
            ("3\nsin(x+0.5)=0,x,0,6.283\n", "Complex solve: shifted radian sine", "x ="),
            ("1\nsin(4*x+1)=2*sin(2*x+0.5)*cos(2*x+0.5)\n1\n", "Complex proof: offset doubled angle", "LHS = RHS"),
            ("1\ncos(4*x-2)=cos(2*x-1)^2-sin(2*x-1)^2\n1\n", "Complex proof: offset cosine double-angle", "LHS = RHS"),
        ]
        extra_cases.extend(complex_cases)

        extreme_cases = []
        rng = random.Random(20260411)

        proof_args = [
            "x^2+1",
            "2*x+3",
            "3*x-2",
            "4*x+1",
            "2*x^2-3*x+1",
            "3*x^2+2",
        ]
        for arg in proof_args:
            extreme_cases.append((
                f"1\nsin(2*({arg}))=2*sin({arg})*cos({arg})\n1\n",
                f"Extreme proof: sin(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"1\n1-cos(2*({arg}))=2*sin({arg})^2\n1\n",
                f"Extreme proof: 1-cos(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"1\n1+cos(2*({arg}))=2*cos({arg})^2\n1\n",
                f"Extreme proof: 1+cos(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"1\nsec({arg})^2-tan({arg})^2=1\n1\n",
                f"Extreme proof: sec²({arg})-tan²({arg})",
                trig_prove_checker(),
            ))

        transform_args = ["2*x+1", "3*x-2", "4*x+3", "2*x^2+1", "3*x^2-2*x+4"]
        for arg in transform_args:
            extreme_cases.append((
                f"2\n1-cos(2*({arg}))\n2*sin({arg})^2\n",
                f"Extreme transform: 1-cos(2({arg}))",
                trig_transform_checker("sin"),
            ))
            extreme_cases.append((
                f"2\n1+cos(2*({arg}))\n2*cos({arg})^2\n",
                f"Extreme transform: 1+cos(2({arg}))",
                trig_transform_checker("cos"),
            ))
            extreme_cases.append((
                f"2\nsin(2*({arg}))\n2*sin({arg})*cos({arg})\n",
                f"Extreme transform: sin(2({arg}))",
                trig_transform_checker("2*sin"),
            ))

        solve_cases = [
            ("sin(3*x)=sqrt(3)/2,x,0,360", "Extreme solve: sin(3x)=√3/2"),
            ("cos(4*x)=1/2,x,0,360", "Extreme solve: cos(4x)=1/2"),
            ("tan(3*x+30)=1,x,0,360", "Extreme solve: tan(3x+30)=1"),
            ("sin(2*x)=sqrt(2)/2,x,0,360", "Extreme solve: sin(2x)=√2/2"),
            ("cos(3*x-30)=1/2,x,0,360", "Extreme solve: cos(3x-30)=1/2"),
            ("tan(4*x)=sqrt(3),x,0,360", "Extreme solve: tan(4x)=√3"),
        ]
        for eq, label in solve_cases:
            extreme_cases.append((
                f"3\n{eq}\n",
                label,
                trig_solve_checker(),
            ))

        for scale in range(2, 7):
            extreme_cases.append((
                f"1\nsin(2*({scale}*x^2+1))=2*sin({scale}*x^2+1)*cos({scale}*x^2+1)\n1\n",
                f"Extreme proof: quadratic-angle sin double scale={scale}",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"1\n1-cos(2*({scale}*x^2-1))=2*sin({scale}*x^2-1)^2\n1\n",
                f"Extreme proof: quadratic-angle cosine scale={scale}",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"2\nsin(2*({scale}*x^2-1))\n2*sin({scale}*x^2-1)*cos({scale}*x^2-1)\n",
                f"Extreme transform: quadratic-angle sin scale={scale}",
                trig_transform_checker("2*sin"),
            ))
            extreme_cases.append((
                f"1\ntan({scale}*x^2+1)=sin({scale}*x^2+1)/cos({scale}*x^2+1)\n1\n",
                f"Extreme proof: tan({scale}x^2+1)",
                trig_prove_checker(),
            ))

        rewrite_cases = [
            ("4\nsin(2*x)^2+cos(2*x)^2\nsin(2*x)\ncos(2*x)\n\n", "Rewrite: sin(2x)^2+cos(2x)^2", trig_rewrite_checker("= 1")),
            ("4\n1-cos(2*(x+1))\nsin(x+1)^2\ncos(x+1)^2\n\n", "Rewrite: 1-cos(2(x+1))", trig_rewrite_checker("2*sin(1+x)^2")),
        ]

        rng.shuffle(extreme_cases)

        if difficulty in ("all", "hard"):
            self.run_simple_cases("trigProgram.py", p, extra_cases[:100], "contains")
            self.run_simple_cases("trigProgram.py", p, extreme_cases[:100], "contains")
            self.run_simple_cases("trigProgram.py", p, rewrite_cases, "contains")

    def run_derive(self, difficulty="all"):
        p = "Derive"

        tests = [
            ("x^7-3*x^3+5*x-8", "d/dx", "d/dx[x⁷-3x³+5x-8]"),
            ("sin(x)*cos(x)", "d/dx", "d/dx[sinx·cosx]"),
            ("exp(3*x-2)", "d/dx", "d/dx[e^(3x-2)]"),
            ("log(x^2+3*x+5)", "d/dx", "d/dx[ln(x²+3x+5)]"),
            ("sqrt(2*x+9)", "d/dx", "d/dx[√(2x+9)]"),
            ("(x^2+1)/(x+3)", "d/dx", "d/dx[(x²+1)/(x+3)]"),
            ("tan(x)", "d/dx", "d/dx[tanx]"),
            ("sec(x)", "d/dx", "d/dx[secx]"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for expr, check, label in tests:
                out, _ = self.run_cli("deriveProgram.py", f"1\n{expr}\n")
                self.add_test(label, "Error" not in out and (check in out or "dy/dx" in out), out, p)

        # Extreme - log differentiation
        if difficulty in ("all", "medium", "hard"):
            out, _ = self.run_cli("deriveProgram.py", "1\nx^x\n")
            self.add_test("Extreme: d/dx[x^x] (log diff)", "Error" not in out, out, p)

            out, _ = self.run_cli("deriveProgram.py", "1\nx^2+y^2+2*x*y=5\n")
            self.add_test("Extreme: implicit diff", "Error" not in out, out, p)

            out, _ = self.run_cli("deriveProgram.py", "1\nsin(x)^x\n")
            self.add_test("Extreme: d/dx[sin(x)^x]", "Error" not in out, out, p)

            out, _ = self.run_cli("deriveProgram.py", "1\nsin(exp(x^2))\n")
            self.add_test("Extreme: chain rule deep", "Error" not in out, out, p)

        implicit_cases = [
            ("2\nx^2+y^2=25\n", "Implicit: x^2+y^2=25", derive_implicit_checker("-x/y")),
            ("2\nx^2+x*y+y^2=7\n", "Implicit: x^2+xy+y^2=7", derive_implicit_checker("dy/dx")),
            ("2\nsin(x)+cos(y)=1\n", "Implicit: sin(x)+cos(y)=1", derive_implicit_checker("dy/dx")),
        ]

        param_cases = [
            ("3\nt^2+1\nt^3-2*t\n", "Parametric: x=t^2+1, y=t^3-2t", derive_param_checker("(3*t^2-2)/(2*t)")),
            ("3\nsin(t)+t^2\ncos(t)-t\n", "Parametric: x=sin(t)+t^2, y=cos(t)-t", derive_param_checker("(-sin(t)-1)/(cos(t)+2*t)")),
            ("3\nexp(t)+sin(t)\nlog(t)+t^2\n", "Parametric: x=exp(t)+sin(t), y=log(t)+t^2", derive_param_checker("(2*t^2+1)/(t*(exp(t)+cos(t)))")),
        ]

        if difficulty in ("all", "medium", "hard"):
            self.run_simple_cases("deriveProgram.py", p, implicit_cases, "contains")
            self.run_simple_cases("deriveProgram.py", p, param_cases, "contains")

        extra_exprs = []

        for a in range(2, 12):
            extra_exprs.append((f"sin(({a}*x+1)^2)", f"Generated derive: sin(({a}x+1)^2)"))
            extra_exprs.append((f"exp(sin({a}*x))", f"Generated derive: exp(sin({a}x))"))
            extra_exprs.append((f"log(({a}*x^2+3*x+5)^2)", f"Generated derive: log(({a}x^2+3x+5)^2)"))
            extra_exprs.append((f"({a}*x^2+1)/(x+{a})", f"Generated derive: ({a}x^2+1)/(x+{a})"))
            extra_exprs.append((f"sqrt({a}*x+{a+3})", f"Generated derive: sqrt({a}x+{a+3})"))
            extra_exprs.append((f"{a}^(x^2+x)", f"Generated derive: {a}^(x^2+x)"))
            extra_exprs.append((f"(sin(x)+cos(x))^{a}", f"Generated derive: (sinx+cosx)^{a}"))
            extra_exprs.append((f"tan(exp(x/{a}))", f"Generated derive: tan(exp(x/{a}))"))
            extra_exprs.append((f"(x^{a}+1)*exp(x)", f"Generated derive: (x^{a}+1)e^x"))
            extra_exprs.append((f"log(sqrt(x^2+{a}))", f"Generated derive: log(sqrt(x^2+{a}))"))

        extra_exprs.extend([
            ("exp(log((x^2+1)^5))", "Complex derive: exp(log((x^2+1)^5))"),
            ("sin((exp(x)+log(x))^3)", "Complex derive: sin((exp(x)+log(x))^3)"),
            ("(exp(2*x)+sin(3*x))^5", "Complex derive: (exp(2x)+sin(3x))^5"),
            ("log((sin(2*x)^2+cos(2*x)^2)^3)", "Complex derive: log(identity^3)"),
            ("(x^3+1)^4/(exp(x)+1)", "Complex derive: quotient with quartic-over-exp"),
            ("sqrt(exp(x^2)+log(x)^2)", "Complex derive: sqrt(exp(x^2)+log(x)^2)"),
            ("tan((x^2+1)/(x+1))", "Complex derive: tan(rational)"),
            ("3^(sin(x^2)+exp(x))", "Complex derive: 3^(sin(x^2)+exp(x))"),
            ("(sin(4*x-1)+cos(3*x+2))^6", "Complex derive: shifted trig sum power"),
            ("log(exp(sqrt(x^2+5)))", "Complex derive: log(exp(sqrt(x^2+5)))"),
            ("exp(sin(log(x)^2))", "Complex derive: exp(sin(log(x)^2))"),
            ("((x^2+1)/(x^3+2))^5", "Complex derive: rational power"),
        ])

        extreme_exprs = []
        rng = random.Random(20260412)

        for a, b, c in [(2, 1, 3), (3, 2, 4), (4, 1, 5), (5, 3, 2), (6, 5, 7)]:
            extreme_exprs.extend([
                (f"exp(sin(({a}*x^2+{b})^2))", f"Extreme derive: exp(sin(({a}x^2+{b})^2))"),
                (f"(({a}*x^2+1)/(x+{c}))^4", f"Extreme derive: (({a}x^2+1)/(x+{c}))^4"),
                (f"(x^2+{b})^(sin({a}*x))", f"Extreme derive: (x^2+{b})^(sin({a}x))"),
                (f"log((x^2+{b})^(x^3+{c}))", f"Extreme derive: log((x^2+{b})^(x^3+{c}))"),
                (f"((sin({a}*x)+cos({b}*x))/(x^2+{c}))^3", f"Extreme derive: trig quotient cube a={a},b={b},c={c}"),
                (f"(exp({a}*x)+sin({b}*x))^5", f"Extreme derive: (exp({a}x)+sin({b}x))^5"),
                (f"tan(({a}*x^2+{b})/(x+{c}))", f"Extreme derive: tan(rational) a={a},b={b},c={c}"),
                (f"(x^2+1)^(exp(x)+sin({a}*x))", f"Extreme derive: (x^2+1)^(exp(x)+sin({a}x))"),
                (f"(exp(x^2)+log(x)^{a})/(sin({b}*x)+1)", f"Extreme derive: mixed quotient a={a},b={b}"),
                (f"sqrt(exp({a}*x^2)+log(x)^{b}+sin({c}*x)^2)", f"Extreme derive: sqrt mixed a={a},b={b},c={c}"),
            ])

        rng.shuffle(extreme_exprs)

        if difficulty in ("all", "hard"):
            hard_cases = []
            for expr, label in extra_exprs[:100]:
                hard_cases.append((f"1\n{expr}\n", label, derive_checker()))
            for expr, label in extreme_exprs[:100]:
                hard_cases.append((f"1\n{expr}\n", label, derive_checker()))
            self.run_simple_cases("deriveProgram.py", p, hard_cases, "contains")

    def run_integrate(self, difficulty="all"):
        p = "Integrate"

        tests = [
            ("x^7-3*x^2+4", "+ C", "∫x⁷-3x²+4 dx"),
            ("1/x", "ln|", "∫1/x dx"),
            ("2^x", "ln", "∫2^x dx"),
            ("exp(x)", "+ C", "∫e^x dx"),
            ("sin(x)", "+ C", "∫sinx dx"),
            ("cos(x)", "+ C", "∫cosx dx"),
            ("tan(x)", "+ C", "∫tanx dx"),
            ("sec(x)^2", "+ C", "∫sec²x dx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for expr, check, label in tests:
                out, _ = self.run_cli("intProgram.py", f"1\n{expr}\n1\n")
                self.add_test(label, check in out, out, p)

        # Substitution tests
        subs = [
            ("1\n(3*x^2+1)/(x^3+x+7)\n4\nx^3+x+7\n", "x^3+x+7", "Sub: ∫(3x²+1)/(x³+x+7) dx"),
            ("1\ncos(3*x+1)\n4\n3*x+1\n", "3*x+1", "Sub: ∫cos(3x+1) dx"),
            ("1\n(5*x+1)^4\n4\n5*x+1\n", "5*x+1", "Sub: ∫(5x+1)⁴ dx"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in subs:
                out, _ = self.run_cli("intProgram.py", inp)
                self.add_test(label, check in out or "+ C" in out, out, p)

        # By parts tests
        parts = [
            ("1\nx*sin(x)\n5\n", "+ C", "By parts: ∫x·sinx dx"),
            ("1\nx*cos(x)\n5\n", "+ C", "By parts: ∫x·cosx dx"),
            ("1\nx*exp(x)\n5\n", "+ C", "By parts: ∫x·e^x dx"),
            ("1\nlog(x)\n5\n", "+ C", "By parts: ∫lnx dx"),
            ("1\nasin(x)\n5\n", "+ C", "By parts: ∫arcsinx dx"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in parts:
                out, _ = self.run_cli("intProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Extreme
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("intProgram.py", "1\nsin(x)^4*cos(x)^2\n1\n")
            self.add_test("Extreme: ∫sin⁴x·cos²x dx", "+ C" in out and "sin" in out, out, p)

            out, _ = self.run_cli("intProgram.py", "1\n(x^3+2*x^2-x+1)/((x-1)^2*(x+2))\n1\n")
            self.add_test("Extreme: partial fractions", "+ C" in out or "partial" in out.lower(), out, p)

        extra_cases = []

        for a in range(2, 12):
            extra_cases.append((f"1\n({a}*x+1)^3\n1\n", f"Generated integrate: ({a}x+1)^3", "+ C"))
            extra_cases.append((f"1\nexp({a}*x-1)\n1\n", f"Generated integrate: exp({a}x-1)", "+ C"))
            extra_cases.append((f"1\nsin({a}*x+1)\n1\n", f"Generated integrate: sin({a}x+1)", "+ C"))
            extra_cases.append((f"1\ncos({a}*x-1)\n1\n", f"Generated integrate: cos({a}x-1)", "+ C"))
            extra_cases.append((f"1\n1/({a}*x+3)\n1\n", f"Generated integrate: 1/({a}x+3)", "ln"))
            extra_cases.append((f"1\n({2*a}*x+{a})/(x^2+{a}*x+5)\n1\n", f"Generated integrate: derivative-over-function a={a}", "ln"))
            extra_cases.append((f"1\nx*exp({a}*x)\n5\n", f"Generated parts: x*exp({a}x)", "+ C"))
            extra_cases.append((f"1\nx*sin({a}*x)\n5\n", f"Generated parts: x*sin({a}x)", "+ C"))
            extra_cases.append((f"1\nx*cos({a}*x)\n5\n", f"Generated parts: x*cos({a}x)", "+ C"))
            extra_cases.append((f"1\ncos({a}*x+1)\n4\n{a}*x+1\n", f"Generated sub: cos({a}x+1)", "+ C"))
            extra_cases.append((f"1\n({a}*x+1)^4\n4\n{a}*x+1\n", f"Generated sub: ({a}x+1)^4", "+ C"))

        extra_cases.extend([
            ("1\n(exp(2*x)+sin(x))*(2*exp(2*x)+cos(x))/((exp(2*x)+sin(x))^2+1)\n1\n", "Complex integrate: mixed derivative-over-function", "ln"),
            ("1\nx^2*exp(3*x)\n5\n", "Complex parts: x^2*exp(3x)", "+ C"),
            ("1\nx^2*sin(2*x)\n5\n", "Complex parts: x^2*sin(2x)", "+ C"),
            ("1\n(log(x))^2\n5\n", "Complex parts: (log(x))^2", "+ C"),
            ("1\n(exp(x))/(exp(2*x)+1)\n4\nexp(x)\n", "Complex sub: exp(x)/(exp(2x)+1)", "+ C"),
            ("1\n(6*x+2)/(3*x^2+2*x+7)\n1\n", "Complex integrate: derivative-over-quadratic", "ln"),
            ("1\nsin(4*x-3)\n1\n", "Complex integrate: sin(4x-3)", "+ C"),
            ("1\ncos(5*x+2)\n1\n", "Complex integrate: cos(5x+2)", "+ C"),
            ("1\n1/(x*(log(x)))\n1\n", "Complex integrate: 1/(x ln x)", "ln"),
            ("1\n(2*x+1)/(sqrt(x^2+x+5))\n1\n", "Complex integrate: linear over sqrt(quadratic)", "+ C"),
            ("1\n(exp(x)+1)^5*exp(x)\n4\nexp(x)+1\n", "Complex sub: (exp(x)+1)^5 exp(x)", "+ C"),
            ("1\nsin(3*x+1)^5*cos(3*x+1)\n4\nsin(3*x+1)\n", "Complex sub: sin^5(...)cos(...)", "+ C"),
        ])
        extreme_cases = []
        rng = random.Random(20260413)

        for a in range(2, 7):
            extreme_cases.extend([
                (f"1\nx^3*exp({a}*x)\n5\n", f"Extreme parts: x^3*exp({a}x)", integrate_checker()),
                (f"1\nx^3*sin({a}*x)\n5\n", f"Extreme parts: x^3*sin({a}x)", integrate_checker()),
                (f"1\nx^3*cos({a}*x)\n5\n", f"Extreme parts: x^3*cos({a}x)", integrate_checker()),
                (f"1\nx^2*exp(x)*sin({a}*x)\n5\n", f"Extreme parts loop: x^2*exp(x)*sin({a}x)", integrate_checker()),
                (f"1\nx^2*exp(x)*cos({a}*x)\n5\n", f"Extreme parts loop: x^2*exp(x)*cos({a}x)", integrate_checker()),
                (f"1\nx^{a}*log(x)\n5\n", f"Extreme parts: x^{a}*log(x)", integrate_checker()),
                (f"1\n({2*a}*x+{a+1})/(x^2+{2*a+1}*x+{a*(a+1)})\n1\n", f"Extreme rational: factorable quadratic a={a}", integrate_checker("ln")),
                (f"1\n({2*a}*x+{a+1})/(x^2+{2*a}*x+{a*a})\n1\n", f"Extreme rational: repeated-root quadratic a={a}", integrate_checker("ln")),
                (f"1\n1/(x^2-{a*a})\n1\n", f"Extreme rational: 1/(x^2-{a*a})", integrate_checker("ln")),
                (f"1\n1/({a*a}-x^2)\n1\n", f"Extreme rational: 1/({a*a}-x^2)", integrate_checker("ln")),
                (f"1\n({2*a}*x+{a+1})/(sqrt(x^2+{a+1}*x+{a*a+3}))\n1\n", f"Extreme sqrt-quadratic a={a}", integrate_checker("sqrt")),
                (f"1\n(exp({a}*x)+1)^4*exp({a}*x)\n4\nexp({a}*x)+1\n", f"Extreme sub: (exp({a}x)+1)^4 exp({a}x)", integrate_checker()),
                (f"1\nsin({a}*x+1)^5*cos({a}*x+1)\n4\nsin({a}*x+1)\n", f"Extreme sub: sin^5({a}x+1)cos({a}x+1)", integrate_checker()),
            ])

        extra_integrals = [
            ("1\nexp(x)*sin(x)\n5\n", "Extreme standard loop: exp(x)*sin(x)", integrate_checker()),
            ("1\nexp(x)*cos(x)\n5\n", "Extreme standard loop: exp(x)*cos(x)", integrate_checker()),
            ("1\nx^2*exp(x)*sin(x)\n5\n", "Extreme mixed loop: x^2*exp(x)*sin(x)", integrate_checker()),
            ("1\nx^2*exp(x)*cos(x)\n5\n", "Extreme mixed loop: x^2*exp(x)*cos(x)", integrate_checker()),
            ("1\n(4*x+5)/(x^2+5*x+6)\n1\n", "Extreme direct: factorable linear/quadratic", integrate_checker("ln")),
            ("1\n(4*x+5)/(x^2+4*x+4)\n1\n", "Extreme direct: repeated-root linear/quadratic", integrate_checker("ln")),
            ("1\n(2*x+3)/(sqrt(x^2+3*x+7))\n1\n", "Extreme direct: derivative over sqrt(quadratic)", integrate_checker("sqrt")),
        ]
        extreme_cases.extend(extra_integrals)
        rng.shuffle(extreme_cases)
        if difficulty in ("all", "hard"):
            hard_cases = []
            for inp, label, check in extra_cases[:100]:
                token = check.lower()
                checker = integrate_checker("ln") if "ln" in token else integrate_checker()
                hard_cases.append((inp, label, checker))
            hard_cases.extend(extreme_cases[:100])
            self.run_simple_cases("intProgram.py", p, hard_cases, "contains")

    def run_suvat(self, difficulty="all"):
        p = "SUVAT"

        try:
            import SUVATprogram as sp

            def fraction_text(value):
                frac = value if isinstance(value, Fraction) else Fraction(value).limit_denominator()
                if frac.denominator == 1:
                    return str(frac.numerator)
                return f"{frac.numerator}/{frac.denominator}"

            def suvat_num(value):
                frac = value if isinstance(value, Fraction) else Fraction(value).limit_denominator()
                return sp.num(frac.numerator, frac.denominator)

            tests = [
                (None, sp.num(5), None, sp.num(2), sp.num(3), 's', '24', "Find s: u=5, a=2, t=3"),
                (None, sp.num(0), None, sp.num(4), sp.num(5), 's', '50', "Find s: u=0, a=4, t=5"),
                (None, sp.num(-3), None, sp.num(2), sp.num(4), 's', '4', "Find s: u=-3, a=2, t=4"),
                (sp.num(20), None, sp.num(14), sp.num(2), sp.num(4), 'u', '6', "Find u: s=20, v=14, a=2, t=4"),
                (sp.num(15), None, sp.num(9), sp.num(-2), sp.num(3), 'u', '15', "Find u: s=15, v=9, a=-2, t=3"),
                (None, sp.num(7), None, sp.num(3), sp.num(2), 'v', '13', "Find v: u=7, a=3, t=2"),
                (None, sp.num(0), None, sp.num(9), sp.num(2), 'v', '18', "Find v: u=0, a=9, t=2"),
            ]

            if difficulty in ("all", "easy", "medium", "hard"):
                for s, u, v, a, t, target, expected, label in tests:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)
        except Exception as e:
            self.add_test("SUVAT import", False, str(e), p)

        # Extreme - projectile
        if difficulty in ("all", "medium", "hard"):
            out, _ = self.run_cli("SUVATprogram.py", "25\n0\n9.8\n4\n\n")
            self.add_test("Extreme: projectile (u=25, a=9.8, t=4)", "s =" in out or "h =" in out.lower(), out, p)

        try:
            import SUVATprogram as sp

            extra_cases = []

            for u_val in range(-4, 6):
                for a_val in range(-3, 4):
                    if a_val == 0:
                        continue
                    t_val = abs(a_val) + 2
                    s_expected = Fraction(u_val * t_val, 1) + Fraction(a_val * t_val * t_val, 2)
                    v_expected = Fraction(u_val + a_val * t_val, 1)
                    extra_cases.append((
                        None, sp.num(u_val), None, sp.num(a_val), sp.num(t_val), 's', fraction_text(s_expected),
                        f"Generated SUVAT: find s from u={u_val}, a={a_val}, t={t_val}"
                    ))
                    extra_cases.append((
                        None, sp.num(u_val), None, sp.num(a_val), sp.num(t_val), 'v', fraction_text(v_expected),
                        f"Generated SUVAT: find v from u={u_val}, a={a_val}, t={t_val}"
                    ))

            for v_val in range(4, 16):
                for a_val in range(1, 6):
                    t_val = a_val + 1
                    u_expected = v_val - a_val * t_val
                    s_val = Fraction((u_expected + v_val) * t_val, 2)
                    extra_cases.append((
                        suvat_num(s_val), None, sp.num(v_val), sp.num(a_val), sp.num(t_val), 'u',
                        fraction_text(Fraction(u_expected, 1)),
                        f"Generated SUVAT: find u from s={s_val}, v={v_val}, a={a_val}, t={t_val}"
                    ))

            extreme_fractional_cases = []
            fractional_us = [Fraction(-5, 2), Fraction(-3, 2), Fraction(1, 2), Fraction(3, 2), Fraction(5, 2)]
            fractional_as = [Fraction(-7, 3), Fraction(-3, 2), Fraction(-1, 2), Fraction(1, 2), Fraction(5, 3), Fraction(7, 2)]
            fractional_ts = [Fraction(3, 2), Fraction(2, 1), Fraction(5, 2), Fraction(7, 3)]

            for u_val in fractional_us:
                for a_val in fractional_as:
                    for t_val in fractional_ts:
                        v_val = u_val + a_val * t_val
                        s_val = u_val * t_val + Fraction(1, 2) * a_val * t_val * t_val
                        extreme_fractional_cases.append((
                            None, suvat_num(u_val), None, suvat_num(a_val), suvat_num(t_val), 's',
                            fraction_text(s_val),
                            f"Extreme SUVAT: find s from u={u_val}, a={a_val}, t={t_val}"
                        ))
                        extreme_fractional_cases.append((
                            None, suvat_num(u_val), None, suvat_num(a_val), suvat_num(t_val), 'v',
                            fraction_text(v_val),
                            f"Extreme SUVAT: find v from u={u_val}, a={a_val}, t={t_val}"
                        ))
                        if a_val != 0:
                            extreme_fractional_cases.append((
                                suvat_num(s_val), None, suvat_num(v_val), suvat_num(a_val), suvat_num(t_val), 'u',
                                fraction_text(u_val),
                                f"Extreme SUVAT: recover u from s={s_val}, v={v_val}, a={a_val}, t={t_val}"
                            ))

            if difficulty in ("all", "hard"):
                for s, u, v, a, t, target, expected, label in extra_cases[:100]:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)
                for s, u, v, a, t, target, expected, label in extreme_fractional_cases[:100]:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)
        except Exception as e:
            self.add_test("Generated SUVAT batch", False, str(e), p)


def main():
    parser = argparse.ArgumentParser(description="CASIO Test Suite")
    parser.add_argument("--plain", action="store_true", help="Plain output mode")
    args = parser.parse_args()

    if not TEXTUAL_AVAILABLE:
        print("Textual required. Install: pip install textual")
        sys.exit(1)

    app = CASIOApp()
    app.run()

if __name__ == "__main__":
    main()
