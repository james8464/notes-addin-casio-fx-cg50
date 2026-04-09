import ast
from pathlib import Path


ROOT = Path("/Users/james/Developer/Python/CASIO/src/Math")
PROGRAM_FILES = {
    "algebraProgram": ROOT / "algebraProgram.py",
    "deriveProgram": ROOT / "deriveProgram.py",
    "intProgram": ROOT / "intProgram.py",
    "trigProgram": ROOT / "trigProgram.py",
    "SUVATprogram": ROOT / "SUVATprogram.py",
}


def imported_program_modules(tree):
    found = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                found.add(alias.name.split(".")[0])
        elif isinstance(node, ast.ImportFrom) and node.module:
            found.add(node.module.split(".")[0])
    return found


def main():
    failures = []
    program_names = set(PROGRAM_FILES)
    for name, path in PROGRAM_FILES.items():
        tree = ast.parse(path.read_text(), filename=str(path))
        imports = imported_program_modules(tree)
        disallowed = sorted((imports & program_names) - {name})
        if disallowed:
            failures.append(f"{name} imports {', '.join(disallowed)}")

    if failures:
        print("Program independence check failed:")
        for item in failures:
            print(" -", item)
        raise SystemExit(1)

    print("Program independence check passed.")
    for name in sorted(PROGRAM_FILES):
        print(" -", name, "is self-contained.")


if __name__ == "__main__":
    main()
