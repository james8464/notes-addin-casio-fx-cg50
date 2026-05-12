#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


@dataclass(frozen=True)
class Case:
    tag: str
    flag: str
    expr: str
    must: tuple[str, ...]
    forbid: tuple[str, ...] = ()
    min_lines: int = 3


GLOBAL_FORBID = (
    "ERR:",
    "Unexpected token",
    "Traceback",
    "could not get working",
    "Chk: ok",
    "Answer: int(",
    "Answer: d/dx(",
)


def compact(s: str) -> str:
    s = s.replace(" ", "").replace("\t", "").replace("\r", "").replace("\n", "").lower()
    for a, b in (
        ("ln(", "log("),
        ("answer:", ""),
        ("result:", ""),
        ("therefore", ""),
        ("hence", ""),
        ("then", ""),
        ("so", ""),
        ("integral", "int"),
    ):
        s = s.replace(a, b)
    return s


def missing_markers(out: str, case: Case) -> list[str]:
    norm = compact(out)
    out_l = out.lower()
    missing: list[str] = []
    for m in case.must:
        ml = m.strip().lower()
        if ml == "equivalent":
            if "not equivalent" in out_l or "equivalent" not in out_l:
                missing.append(m)
            continue
        if ml == "not equivalent":
            if "not equivalent" not in out_l:
                missing.append(m)
            continue
        if compact(m) not in norm:
            missing.append(m)
    return missing


def run(flag: str, expr: str) -> tuple[int, str]:
    p = subprocess.run(
        [str(HOST), "--" + flag, expr],
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=20,
    )
    return p.returncode, p.stdout + p.stderr


def base_cases() -> list[Case]:
    c: list[Case] = []

    # Algebra solve.
    for item in [
        ("2*x+3=11,method=linear", ("x = [4]",)),
        ("x^2-6*x+8=0", ("(x - 3)^2 = 1", "x - 3 = +/-1", "x = [4, 2]")),
        ("x^2-5*x+6=0,method=factor", ("(x - 3)*(x - 2)", "x = [3, 2]")),
        ("x^2-6*x+5=0,method=complete_square", ("(x - 3)^2 - 4 = 0", "x = [5, 1]")),
        ("x^4+3*x^2-26=0", ("u = x^2", "x =")),
        ("x^4-5*x^2+4=0,method=substitution", ("u = x^2", "x = 2", "x = -2", "x = 1", "x = -1"), ("sqrt(4)", "sqrt(1)")),
        ("(x+1)/(x-2)+(x-2)/(x+1)=5,method=clear_denoms", ("Domain:", "x =")),
        ("sqrt(x+5)-sqrt(x-3)=2,method=auto", ("Domain:", "sqrt(x + 5) = 2 + sqrt(x - 3)", "(x + 5) - (x - 3) - 4", "x = [4]")),
        ("x+2*sqrt(x)-8=0", ("u = sqrt(x)", "u^2 + 2*u - 8 = 0", "x = 4")),
        ("x+7=5*sqrt(x)", ("u = sqrt(x)", "b^2 - 4ac = -3 < 0", "x = []"), ("*i",)),
        ("(x^2-5*x+7)^(x^2-9*x+20)=1", ("base = 1", "exponent = 0", "x = [2, 3, 4, 5]")),
        ("abs(2*x+1)+9<4*x", ("abs", "x > 5/2")),
        ("2^(2*x)-5*2^x+4=0,method=log_exp", ("u=a^x", "x =")),
        ("solve(log(2,x)+log(4,x)=6,x,method=log_exp)", ("u = log(2,x)", "x = 16")),
        ("solve(log(2,x)+log(x,2)=5/2,x,method=log_exp)", ("1/u", "x = sqrt(2) or 4")),
        ("solve(log(3,x)+log(x,27)=4,x,method=log_exp)", ("3/u", "x = 3 or 27")),
        ("solve(log(2,x+3)+log(2,x+10)=2+2*log(2,x),x,method=log_exp)", ("log(2,(x+3)*(x+10))", "x = [6]")),
        ("solve(log(3,x^2*y)-2*log(3,x)=5,y,method=log_exp)", ("y = 243",)),
    ]:
        expr, must, *rest = item
        forbid = rest[0] if rest else ("log/exp laws to combine",)
        c.append(Case("alg_solve", "alg", expr, must, forbid))

    # Algebra manipulations.
    for expr, must in [
        ("factor(x^2-5*x+6)", ("(x - 3)*(x - 2)",)),
        ("expand((2*x-3)^4)", ("(2*x - 3)^4", "16*x^4")),
        ("(m*x+n)^2,method=expand", ("(m*x + n)^2", "2*m", "n", "x")),
        ("x^2+2*x+1,method=collect", ("x^2 + 2*x + 1",)),
        ("x^2+2*x+1,method=canonical", ("x^2 + 2*x + 1", "= x^2 + 2*x + 1")),
        ("complete_square(2*x^2+8*x+3)", ("h =", "k =", "Answer:")),
        ("complete_square((2*x+4)^2)", ("h = 2", "k = 0", "4*(x + 2)^2")),
        ("1/(sqrt(x)+1),method=rationalise", ("*(sqrt(x) - 1)/(sqrt(x) - 1)", "(sqrt(x) - 1)/(x - 1)")),
        ("pf((x^2+5*x+6)/(x^2*(x+1)))", ("A/x^2", "A=", "B=", "C=")),
        ("compare((x^2-1)/(x-1),x+1)", ("E1 =", "E2 =", "Equivalent")),
        ("compare(x^2=1,(x-1)*(x+1)=0)", ("E1-R1", "E2-R2", "equivalent")),
        ("compare(1/(x-x),2/(x-x))", ("not equivalent",)),
        ("compare(log(2,x^2),2*log(2,x))", ("not equivalent",)),
        ("compare(log(x^2)=0,2*log(x)=0)", ("not equivalent",)),
        ("compare(log(x^2)=0,x^2=1)", ("equivalent",)),
        ("compare(exp(log(x)),x)", ("not equivalent",)),
        ("compare(exp(log(x))=2,x=2)", ("equivalent",)),
        ("compare(log(exp(x)),x)", ("equivalent",)),
        ("compare(sqrt((x-1)^2),x-1)", ("not equivalent",)),
        ("compare(sqrt((x-1)^2),abs(x-1))", ("equivalent",)),
        ("compare(asin(sin(x)),x)", ("not equivalent",)),
        ("compare(tan(atan(x)),x)", ("equivalent",)),
        ("compare(atan(tan(x)),x)", ("not equivalent",)),
        ("compare(abs(x)^2,x^2)", ("equivalent",)),
        ("compare(abs(x),x)", ("not equivalent",)),
        ("compare((x^2-4)/(x-2)=0,x+2=0)", ("equivalent",)),
        ("compare(log(2,x^2+4*x+3)=4+log(2,x^2+x),(x^2+4*x+3)/(x^2+x)=16)", ("equivalent",)),
        ("compare(log(x)=log(y),x=y)", ("equivalent",)),
        ("compare(exp(x)=exp(y),x=y)", ("equivalent",)),
        ("compare(log(2,x)+log(2,y)=3,x*y=8)", ("equivalent",)),
        ("compare(log(3,x^2*y)-2*log(3,x)=5,y=243)", ("equivalent",)),
        ("compare(sqrt(x)=sqrt(y),x=y)", ("equivalent",)),
        ("compare(asin(x)=asin(y),x=y)", ("equivalent",)),
        ("compare(acos(x)=acos(y),x=y)", ("equivalent",)),
        ("compare(atan(x)=atan(y),x=y)", ("equivalent",)),
        ("compare(abs(x)=abs(y),x^2=y^2)", ("equivalent",)),
        ("compare(1/x=1/y,x=y)", ("equivalent",)),
        ("compare(x^3=y^3,x=y)", ("equivalent",)),
        ("transform(x^2-1,(x-1)*(x+1))", ("source-target = 0", "(x - 1)*(x + 1)")),
        ("transform(x^2-1,x^2+1)", ("source-target =", "not equivalent")),
        ("transform(sqrt((x-1)^2),x-1)", ("not equivalent",)),
        ("transform(sqrt((x-1)^2),abs(x-1))", ("source-target = 0", "abs(x - 1)")),
        ("transform(exp(log(x)),x)", ("not equivalent",)),
        ("transform(log(exp(x)),x)", ("source-target = 0", "x")),
        ("transform(log(x^2)=0,x^2=1)", ("source-target = 0", "x^2 = 1")),
        ("transform(log(x)=log(y),x=y)", ("source-target = 0", "x = y")),
        ("transform(exp(x)=exp(y),x=y)", ("source-target = 0", "x = y")),
        ("transform(log(2,x)+log(2,y)=3,x*y=8)", ("source-target = 0", "x*y = 8")),
        ("transform(log(3,x^2*y)-2*log(3,x)=5,y=243)", ("source-target = 0", "y = 243")),
        ("transform(sqrt(x)=sqrt(y),x=y)", ("source-target = 0", "x = y")),
        ("transform(asin(x)=asin(y),x=y)", ("source-target = 0", "x = y")),
        ("transform(atan(x)=atan(y),x=y)", ("source-target = 0", "x = y")),
        ("transform(abs(x)=abs(y),x^2=y^2)", ("source-target = 0", "x^2 = y^2")),
        ("transform(1/x=1/y,x=y)", ("source-target = 0", "x = y")),
        ("transform(x^3=y^3,x=y)", ("source-target = 0", "x = y")),
        ("transform((x+1)^2=x^2+2*x+1)", ("source-target = 0", "x^2 + 2*x + 1")),
        ("rewrite((4*x-3)^2+4*x+5,4*x-3)", ("u = 4*x - 3", "u^2 + u + 8", "(4*x - 3)^2 + (4*x - 3) + 8")),
        ("rewrite((-2*x+6)^8,(-2*x+6)^4)", ("u = (-2*x + 6)^4", "u^2", "((-2*x + 6)^4)^2")),
        ("fitconst((a*x+b)*(x-2)+c*(x+1)^2=4*x^2+6*x-1,[a,b,c])", ("x = 0:", "a = 1", "b = 2", "c = 3")),
        ("rewrite(sqrt(12+sqrt(140)))", ("m+n=12", "Answer: sqrt(7) + sqrt(5)")),
        ("rewrite(sin(x)^4+cos(x)^4,sin(x)^2*cos(x)^2)", ("(sin(x)^2 + cos(x)^2)^2", "1 - 2*sin(x)^2*cos(x)^2")),
        ("binomial(sqrt(1+2*x),x,3)", ("T_r = C(n,r)*u^r", "(1+u)^n", "Valid", "x^3")),
        ("newton(x^3-2*x-5,x,2,3)", ("Newton", "x_", "Answer:")),
        ("inverse((x+3)/2)", ("x = f(y)", "f^-1")),
        ("inverse(5*x-1)", ("f^-1(x) = x/5 + 1/5",)),
    ]:
        min_lines = 1 if expr.startswith("factor(") or "method=collect" in expr or "method=canonical" in expr else 3
        c.append(Case("alg_manip", "alg", expr, must, min_lines=min_lines))
    c.append(Case("alg_manip", "trig", "(cosec(x)^2-cot(x)^2)*exp(x),method=manip_trig", ("e^(x)",), min_lines=1))

    # Domain/range.
    for expr, must in [
        ("domain(sqrt(log(1/2,x-1)))", ("base = 1/2", "1 < x <= 2")),
        ("domain(arcsin((x-1)/3))", ("-1 <=", "Answer:")),
        ("domain(1/(x^2-4))", ("!= 0", "Answer:")),
        ("range(abs(x-1)+abs(x-2))", ("min y = 1", "y >= 1")),
        ("range(sqrt(abs(3*x+1)+1))", ("abs(3*x + 1) >= 0", "y >= 1")),
        ("range(x/(1+x^2))", ("Range:", "-1/2 <= y <= 1/2")),
        ("range((x-1)/(x+1))", ("y != 1",)),
        ("range((2*x+3)/(x-4))", ("y != 2",)),
        ("range(1/(2-cos(3*x)))", ("Range:", "1/3 <= y <= 1")),
        ("range(1/(u+1),u,(1),(9))", ("y(1)=1/2", "1/10 <= y <= 1/2")),
        ("range(sqrt(u+1),u,(2),6)", ("y(2)=sqrt(3)", "sqrt(3) <= y <= sqrt(7)")),
        ("range(x^3,x)", ("odd power", "all real y")),
        ("range(log(10,abs(3*x+1)+3))", ("log monotonicity", "Range:")),
    ]:
        c.append(Case("domain_range", "alg", expr, must))

    # Differentiation.
    for expr, must in [
        ("(2*x+ln(x))^3,x", ("u = 2*x + log(x)", "du/dx", "dy/dx")),
        ("x^2*sin(x),x,method=product", ("f1 =", "f2 =", "dy/dx")),
        ("(x^2+1)/(x-1),x,method=quotient", ("u =", "v =", "dy/dx")),
        ("x^sin(x),x,method=logdiff", ("ln(y)", "dy/dx")),
        ("ln(x+y)=x*y,x,method=implicit", ("(1+dy/dx)/(x+y)", "dy/dx*(1-x*(x+y))", "dy/dx")),
        ("1/(2*x+1)+1/(y+1)=x^2,x,method=implicit", ("* (2*x + 1)*(y + 1)", "dy/dx")),
        ("x=t^2+1/t,y=t^2-1/t,t,x,method=param", ("dx/dt", "dy/dt", "dy/dx")),
        ("mode:6,x^2", ("[f(x+h)-f(x)]/h", "h->0")),
        ("arccos(cos(2*x+pi/6)),x", ("u = cos", "dy/dx")),
        ("log(abs(3*x+1)+2),x", ("d(abs(u))/dx", "dy/dx")),
    ]:
        c.append(Case("derive", "derive", expr, must, ("pick rule",)))

    # Integration.
    for expr, must in [
        ("x^3*e^(2*x),method=di", ("D:", "I:", "Signs:", "+ C")),
        ("x^2*cos(3*x),method=di", ("D:", "I:", "Signs:", "+ C")),
        ("e^(2*x)*cos(3*x),method=parts", ("u = cos(3*x)", "J =", "13/4*I")),
        ("asin((x-3)/8),method=parts", ("u=asin(w)", "dv=dw", "sqrt(1-w^2)")),
        ("acos((x-1)/3),method=parts", ("u=acos(w)", "w=(x - 1)/3", "+ C")),
        ("atan(2*x-3),method=parts", ("u=atan(w)", "dv=dw", "+ C")),
        ("((x^2-1)/(x-1))/(x+1),method=auto", ("N = x^2 - 1", "D = x^2 - 1", "x + C")),
        ("((x)^4+2*x^2+1)/(x^2+1),method=div", ("Divide: N/D = x^2 + 1", "+ C")),
        ("(5*x+7)/((x-1)*(x^2+4)),method=pf", ("A/(x-1)", "A=12/5", "+ C")),
        ("(3*x^2+5*x+7)/((x-1)^2*(x^2+1)),method=pf", ("(x - 1)^2", "coefficient equations", "+ C")),
        ("(x^2+1)/(x^4+1),method=pf", ("Factor x^4+1", "coeffs", "+ C")),
        ("(x^2+1)/(x^4+x^2+1),method=sub", ("N,D / x^2", "u=x-1/x", "+ C")),
        ("(x^2-1)/(x^4+1),method=sub", ("u^2-2", "A=1/(2*sqrt(2))", "+ C")),
        ("1/(2+cos(x)),method=weierstrass", ("t=tan(x/2)", "cos(x)", "+ C")),
        ("defint(ln(sin(x)),x,0,pi/2)", ("x=>pi/2-x", "u = 2x", "-pi*log(2)/2")),
        ("defint(sin(x)^n/(sin(x)^n+cos(x)^n),x,0,pi/2)", ("King property", "2I", "pi/4")),
        ("exp(sqrt(x)),method=sub", ("u=sqrt(x)", "dv=e^u", "+ C")),
        ("x^2/(x*sin(x)+cos(x))^2,method=parts", ("D' = x*cos(x)", "u = x*sec(x)", "+ C")),
    ]:
        c.append(Case("integrate", "int", expr, must, ("Classify the integrand", "No elementary primitive")))

    # Common integration variants.
    for expr, must in [
        ("x^2", ("x^3/3", "+ C")),
        ("1/x", ("log(abs(x))", "+ C")),
        ("log(x)/x", ("u = log(x)", "log(x)^2/2", "+ C")),
        ("sign(2*x-3)", ("u = 2*x - 3", "Int sign(u) du = abs(u)", "abs(2*x - 3)/2 + C")),
        ("sin(3*x+2)", ("cos(3*x + 2)", "+ C")),
        ("cos(x)^3", ("u=sin(x)", "+ C")),
        ("tan(x)^3", ("sec(x)^2 - 1", "+ C")),
        ("1/(x*(x^2+1))", ("A/x", "B", "+ C")),
        ("sqrt(1-x^2)", ("Reference triangle", "+ C")),
        ("sqrt(x^2+1)", ("Reference triangle", "+ C")),
        ("1/(x*sqrt(1+x^n))", ("u=sqrt(1+x^n)", "+ C")),
        ("e^x*(1/x-1/x^2)", ("f(x)=1/x", "+ C")),
    ]:
        c.append(Case("integrate_variants", "int", expr, must, ("No elementary primitive",)))

    # Trig solving/rewrite/proof.
    for item in [
        ("3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform", ("R =", "arccos", "x=arctan(4/3)+arccos(2/5)", "x = [")),
        ("sin(3*x)=sin(x),x,0,2*pi,10,method=identity", ("sin(A) = sin(B):", "x=n*pi", "x = [0, pi/4")),
        ("cos(x)+cos(3*x)=0,x,-180,180", ("-135", "-90", "-45", "45", "90", "135")),
        ("2*sin(x)^2=1+cos(x),x,0,2*pi,10,method=identity", ("u=cos(x)", "alpha = arccos", "x = [")),
        ("2*cos(x)^2+3*cos(x)-2=0,x,0,2*pi,10,method=identity", ("Reject u=-2", "alpha = arccos", "x = [")),
        ("cos(2*x)+cos(x)=2,x,0,180,method=identity", ("c = -3/2 or c = 1", "Reject c = -3/2", "x = [0]"), ("2c^2+c-3",)),
        ("sqrt(1-cos(x))=sin(x),x,0,2*pi,8,method=square_then_check", ("Square", "Check", "Answer:")),
        ("tan(2*x)=sqrt(3),x,0,2*pi,8,method=cast", ("tan(A)", "Base angles", "Answer:")),
        ("1/cos(x)=3,x,0,2*pi,8,method=identity", ("arccos(1/3)", "2*pi - arccos(1/3)")),
        ("cos(x)=0,x,0,360,8,method=identity", ("arccos(0)=pi/2", "90", "270")),
        ("cos(x)*sin(2*x)=0,x,0,180", ("x = [0, 90, 180]",), ("270", "360")),
        ("sin(x)=sqrt(2)/2,x,0,pi", ("arcsin(sqrt(2)/2)=pi/4", "pi/4", "3*pi/4")),
        ("sin(x+y),method=compound_angle", ("sin(A+B)", "sin(x)*cos(y)")),
        ("cos(7*theta),method=auto", ("Chebyshev", "64*c^7")),
        ("tan(3*x),method=auto", ("tan(3", "Answer:")),
        ("sin(x)^2+cos(x)^2,method=sin_cos", ("sin(x)^2 + cos(x)^2", "= 1", "1")),
        ("sin(2*x),method=double_angle", ("sin(2*x) = 2*sin(x)*cos(x)", "2*sin(x)*cos(x)")),
        ("cos(2*x),method=double_angle", ("cos(2*x) = cos(x)^2 - sin(x)^2", "cos(x)^2 - sin(x)^2")),
        ("tan(x),target=sin(x)/cos(x),method=auto", ("tan(u)=sin(u)/cos(u)", "Answer:")),
        ("cos(3*x+2)/sin(3*x+2),target=cot(3*x+2),method=auto", ("cot", "cos(3*x + 2)/sin(3*x + 2)")),
    ]:
        expr, must, *rest = item
        forbid = rest[0] if rest else ("Apply standard identities/rearrangement toward the target",)
        c.append(Case("trig", "trig", expr, must, forbid))

    # Stats/probability/mechanics.
    for item in [
        ("1,2,3,4,method=summary", ("mean =", "median", "sd=")),
        ("binom(10,.5,4)", ("P(X", "B(10")),
        ("normalcdf(0,1,-1.96,1.96)", ("N(0", "Phi")),
        ("reg([1,2,3],[2,4,5])", ("Sxx", "r =")),
        ("1,2,3;-13,-14,-15,method=regression", ("y = -x - 12", "r = -1"), ("+ -",)),
        ("ztest(15,10,5,20,gt)", ("H0", "p = P(Z >")),
    ]:
        expr, must, *rest = item
        forbid = rest[0] if rest else ()
        c.append(Case("stats", "stats", expr, must, forbid, 2))

    for expr, must in [
        ("s=,u=0,v=10,a=2,t=5,target=s,method=suvat", ("s = ut + 1/2at^2", "s = 25")),
        ("s=100,u=,v=20,a=2,t=,target=u,method=suvat", ("u = v - at", "u =")),
        ("s=50,u=5,v=,a=,t=5,find=[v,a],method=suvat", ("v =", "a =")),
        ("s=20,u=3,v=,a=,t=4,target=a,method=suvat", ("a = 2(s-ut)/t^2", "a = 1")),
        ("s=20,u=,v=,a=1,t=4,target=u,method=suvat", ("u = (s - 1/2at^2)/t", "u = 3")),
    ]:
        c.append(Case("suvat", "suvat", expr, must, min_lines=2))

    return c


def generated_cases() -> list[Case]:
    out: list[Case] = []
    # Hundreds of specific variants around supported paths.
    for a in [2, 3, 4, 5, 7, 10]:
        for n in [2, 3, 4, 5]:
            b = a**n
            out.append(Case("gen_log_value", "alg", f"log({a},{b})", ("log(a,b)=c", str(n)), ("log(" + str(b) + ")/log(" + str(a),), 2))
            out.append(Case("gen_log_solve_power", "alg", f"solve(log({a},x)+log({b},x)={n+2},x,method=log_exp)", ("u = log", "x ="), ("log/exp laws to combine",), 5))
    for k in [1, 2, 3, 4, 5, 6, 7, 8]:
        out.append(Case("gen_diff_chain", "derive", f"(2*x+ln(x))^{k+1},x", ("u = 2*x + log(x)", "dy/dx"), ("pick rule",), 4))
        out.append(Case("gen_int_di_exp", "int", f"x^{k}*e^(2*x),method=di", ("D:", "I:", "Signs:", "+ C"), ("No elementary primitive",), 4))
        out.append(Case("gen_trig_poly", "trig", f"{k+1}*cos(x)^2+3*cos(x)-2=0,x,0,2*pi,10,method=identity", ("u=cos(x)", "Answer:"), ("substitution differential",), 4))
    for p in [1, 2, 3, 4, 5, 6, 7, 8]:
        out.append(Case("gen_domain_sqrt_log", "alg", f"domain(sqrt(log(1/{p+1},x-{p})))", ("base =", "Domain:"), (), 3))
        out.append(Case("gen_range_abs", "alg", f"range(abs(x-{p})+abs(x-{p+2}))", ("min y =", "y >="), ("inspect graph",), 3))
    for n in [2, 3, 4, 5, 6, 7, 8]:
        out.append(Case("gen_integral_reverse_chain", "int", f"x*exp(-{n}*x^2)", ("u=", "+ C"), ("No elementary primitive",), 3))
        out.append(Case("gen_integral_trig_power", "int", f"sin(x)^{n},method=trig", ("sin", "+ C"), ("No elementary primitive",), 3))
    for a in [2, 3, 4, 5, 6, 7]:
        out.append(Case("gen_alg_linear", "alg", f"{a}*x+{a+1}={3*a+1},method=linear", ("x =", "["), (), 3))
        out.append(Case("gen_alg_quad", "alg", f"x^2-{a+3}*x+{3*a}=0,method=factor", ("= 0", "x ="), (), 4))
        out.append(Case("gen_complete_square", "alg", f"complete_square({a}*x^2+{2*a}*x+{a+1})", ("h =", "k =", "Answer:"), (), 4))
    for b, c0 in [(3, 4), (5, 12), (8, 15), (7, 24), (20, 21)]:
        out.append(Case("gen_trig_rform", "trig", f"{b}*cos(x)+{c0}*sin(x)=1,x,0,2*pi,8,method=rform", ("R =", "arccos", "x = ["), (), 5))
    for a in [2, 3, 4, 5, 6]:
        out.append(Case("gen_stats_binom", "stats", f"binom({10+a},.5,{a})", ("B(", "P(X"), (), 3))
        out.append(Case("gen_suvat", "suvat", f"s=,u={a},v=,a=2,t={a+1},target=s,method=suvat", ("s = ut + 1/2at^2", "s ="), (), 2))
    return out


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    cases = base_cases() + generated_cases()
    bad = 0
    by_tag: dict[str, list[str]] = {}
    for i, case in enumerate(cases, 1):
        rc, out = run(case.flag, case.expr)
        lines = [x for x in out.splitlines() if x.strip()]
        missing = missing_markers(out, case)
        forbidden = [f for f in GLOBAL_FORBID + case.forbid if f and f in out]
        ok = rc == 0 and not missing and not forbidden and len(lines) >= case.min_lines
        if ok:
            by_tag.setdefault(case.tag, []).append("OK")
            continue
        bad += 1
        by_tag.setdefault(case.tag, []).append("FAIL")
        print(f"FAIL {i} {case.tag} {case.flag} {case.expr}")
        if missing:
            print("  missing:", missing)
        if forbidden:
            print("  forbidden:", forbidden)
        print(out[:1600])
    print("summary")
    for tag in sorted(by_tag):
        vals = by_tag[tag]
        print(f"{tag}: {vals.count('OK')}/{len(vals)}")
    print(f"done bad {bad}/{len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
