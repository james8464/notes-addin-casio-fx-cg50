#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]


def run(cmd: list[str]) -> int:
    p = subprocess.run(cmd, cwd=str(REPO))
    return p.returncode


def main() -> int:
    lowered = [a.lower() for a in sys.argv[1:]]
    if lowered and lowered[-1] == "compile":
        return run(["./compile"])
    if "--fuzz" in sys.argv:
        return run([sys.executable, "c++/tools/fuzz/check_regressions.py"])

    # Build host first (uses pip --user cmake when needed).
    rc = run(["./c++/tools/build_host.sh"])
    if rc != 0:
        print("FAIL: build_host", file=sys.stderr)
        return rc

    checks: list[tuple[str, list[str]]] = [
        ("khicas_reference", [sys.executable, "c++/tools/check_khicas_reference.py"]),
        ("khicas_catalog", [sys.executable, "c++/tools/check_khicas_catalog.py"]),
        ("help_example_insert", [sys.executable, "c++/tools/check_help_example_insert.py"]),
        ("invalid_input_working", [sys.executable, "c++/tools/check_invalid_input_working.py"]),
        ("diff_exam_working", [sys.executable, "c++/tools/check_diff_exam_working.py"]),
        ("math_style_working", [sys.executable, "c++/tools/check_math_style_working.py"]),
        ("exam_working_contracts", [sys.executable, "c++/tools/golden/check_exam_working_contracts.py"]),
        ("help_quality", [sys.executable, "c++/tools/check_help_quality.py"]),
        ("build_packaging", [sys.executable, "c++/tools/check_build_packaging.py"]),
        ("calculator_border", [sys.executable, "c++/tools/check_calculator_border.py"]),
        ("tui_llm_prompt", [sys.executable, "c++/tools/check_tui_llm_prompt.py"]),
        ("catalogue_random_graph", [sys.executable, "c++/tools/check_catalogue_random_graph.py"]),
        ("khicas_working", [sys.executable, "c++/tools/check_khicas_working.py"]),
        ("formatter_memory", [sys.executable, "c++/tools/check_formatter_memory.py"]),
        ("trig_basis", [sys.executable, "c++/tools/check_trig_basis.py"]),
        ("feature_parity", [sys.executable, "c++/tools/check_feature_parity.py"]),
        ("host_smoke", [sys.executable, "c++/tools/golden/check_host_smoke.py"]),
        ("device_scope", [sys.executable, "c++/tools/golden/check_device_scope.py"]),
        ("tests_txt_cases", [sys.executable, "c++/tools/golden/check_tests_txt_cases.py"]),
        ("expr_format", [sys.executable, "c++/tools/golden/compare_expr_format.py"]),
        ("suvat", [sys.executable, "c++/tools/golden/compare_suvat.py"]),
        ("integrate_basic", [sys.executable, "c++/tools/golden/compare_int_basic.py"]),
        ("integrate_more", [sys.executable, "c++/tools/golden/compare_int_more.py"]),
        ("exam_working", [sys.executable, "c++/tools/golden/check_exam_working.py"]),
        ("integration_centurion", [sys.executable, "c++/tools/golden/check_integration_centurion.py"]),
        ("integration_compact_centurion", [sys.executable, "c++/tools/golden/check_integration_compact_centurion.py"]),
        ("integration_dynamic_patterns", [sys.executable, "c++/tools/golden/check_integration_dynamic_patterns.py"]),
        ("working_hardening", [sys.executable, "c++/tools/golden/check_working_hardening.py"]),
        ("infinite_report_regressions", [sys.executable, "c++/tools/golden/check_infinite_report_regressions.py"]),
        ("syllabus_matrix", [sys.executable, "c++/tools/golden/check_syllabus_matrix.py", "--fail-on-gap"]),
        ("extreme_working_sources", [sys.executable, "c++/tools/golden/check_extreme_working_sources.py"]),
        ("madasmaths_paper_scan", [sys.executable, "c++/tools/golden/check_madasmaths_paper_scan.py"]),
        ("madasmaths_full_audit_unit", [sys.executable, "c++/tools/golden/check_madasmaths_full_audit_unit.py"]),
        ("madasmaths_full_audit", [sys.executable, "c++/tools/golden/check_madasmaths_full_audit.py", "--no-render", "--strict-skips"]),
        ("madasmaths_standard_topics_audit", [sys.executable, "c++/tools/golden/check_madasmaths_standard_topics_audit.py"]),
        ("madasmaths_standard_question_corpus", [sys.executable, "c++/tools/golden/check_madasmaths_standard_question_corpus.py"]),
        ("madasmaths_standard_manual_cases", [sys.executable, "c++/tools/golden/check_madasmaths_standard_manual_cases.py"]),
        ("online_paper_manual_cases", [sys.executable, "c++/tools/golden/check_online_paper_manual_cases.py"]),
        ("edexcel_paper1_downloads", [sys.executable, "c++/tools/golden/check_edexcel_paper1_downloads.py"]),
        ("edexcel_paper2_downloads", [sys.executable, "c++/tools/golden/check_edexcel_paper2_downloads.py"]),
        ("mp2_generalized_variants", [sys.executable, "c++/tools/golden/check_mp2_generalized_variants.py"]),
        ("mp2_vwx_manual", [sys.executable, "c++/tools/golden/check_mp2_vwx_manual.py"]),
        ("mp2_yz_manual", [sys.executable, "c++/tools/golden/check_mp2_yz_manual.py"]),
        ("mp2_stu_manual", [sys.executable, "c++/tools/golden/check_mp2_stu_manual.py"]),
        ("mp2_qr_manual", [sys.executable, "c++/tools/golden/check_mp2_qr_manual.py"]),
        ("mp2_nop_manual", [sys.executable, "c++/tools/golden/check_mp2_nop_manual.py"]),
        ("mp2_klm_manual", [sys.executable, "c++/tools/golden/check_mp2_klm_manual.py"]),
        ("mp2_hij_manual", [sys.executable, "c++/tools/golden/check_mp2_hij_manual.py"]),
        ("mp2_efg_manual", [sys.executable, "c++/tools/golden/check_mp2_efg_manual.py"]),
        ("mp2_cd_manual", [sys.executable, "c++/tools/golden/check_mp2_cd_manual.py"]),
        ("mp2_ab_manual", [sys.executable, "c++/tools/golden/check_mp2_ab_manual.py"]),
        ("general_scope", [sys.executable, "c++/tools/golden/check_general_scope.py"]),
        ("rigor_challenge", [sys.executable, "c++/tools/golden/check_rigor_challenge.py"]),
        ("mathematical_singularity", [sys.executable, "c++/tools/golden/check_mathematical_singularity.py"]),
        ("casio_fx_cg50_stress_file", [sys.executable, "c++/tools/golden/check_casio_fx_cg50_stress_file.py"]),
        ("method_forcing", [sys.executable, "c++/tools/golden/check_method_forcing.py"]),
        ("derive_basic", [sys.executable, "c++/tools/golden/compare_derive_basic.py"]),
        ("trig_basic", [sys.executable, "c++/tools/golden/compare_trig_basic.py"]),
        ("algebra_basic", [sys.executable, "c++/tools/golden/compare_algebra_basic.py"]),
        ("fuzz_regressions", [sys.executable, "c++/tools/fuzz/check_regressions.py"]),
        ("stress_crash", [sys.executable, "c++/tools/tests_cpp/run_tests_tui.py", "stress", "crash", "60"]),
    ]
    prizm_g3a = REPO / "c++/prizm/build/CasioCAS.g3a"
    if prizm_g3a.exists():
        checks.insert(2, ("g3a_metadata", [sys.executable, "c++/tools/check_g3a_metadata.py", str(prizm_g3a)]))
        checks.insert(3, ("g3a_working_markers", [sys.executable, "c++/tools/check_g3a_working_markers.py", str(prizm_g3a)]))
        checks.insert(4, ("g3a_size_report", [sys.executable, "c++/tools/size_report.py", "--baseline", "current"]))
    transfer_pak = REPO / "calculator_files/CASIOCAS.PAK"
    if transfer_pak.exists():
        checks.insert(5, ("external_pack", [sys.executable, "c++/tools/check_external_pack.py", str(transfer_pak)]))

    bad = 0
    for name, cmd in checks:
        print(f"== {name} ==")
        rc = run(cmd)
        if rc != 0:
            bad += 1
            print(f"FAIL {name} (exit={rc})", file=sys.stderr)

    if bad:
        print(f"FAILED: {bad} checks", file=sys.stderr)
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
