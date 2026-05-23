#!/usr/bin/env python3
"""Paper 32 audit cases from local Edexcel QP/MS PDFs."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "June 2019 Q1 acceleration i component",
        ["--derive", "6*t,t"],
        ["dy/dt = 6"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 acceleration j component",
        ["--derive", "-5*t^(3/2),t"],
        ["f' = 3/2*sqrt(t)", "dy/dt = -15/2*sqrt(t)"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 acceleration at t=4",
        ["--alg", "evalat(-15/2*sqrt(t),t,4)"],
        ["f(4) = -15"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 position i primitive",
        ["--int", "int(6*t,t)"],
        ["I = Int(6*t) dt", "3*t^2 + C"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 position j primitive",
        ["--int", "int(-5*t^(3/2),t)"],
        ["I = Int(-5*t^(3/2)) dt", "-2*t^(5/2) + C"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 position at t=4 i",
        ["--alg", "-20+3*4^2"],
        ["28"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 position at t=4 j",
        ["--alg", "20-2*32"],
        ["-44"],
        ["ERR:"],
    ),
    (
        "June 2019 Q2 direction time",
        ["--alg", "solve((4-3*T)/(-1+2*T)=-4/3,T)"],
        ["Domain: 2*T - 1 != 0", "T = 8"],
        ["ERR:"],
    ),
    (
        "June 2019 Q2 distance AB",
        ["--alg", "compare(sqrt(12^2+(-8)^2),4*sqrt(13))"],
        ["E1 = sqrt(208)", "E2 = 4*sqrt(13)", "equivalent"],
        ["ERR:"],
    ),
    (
        "June 2019 Q3 connected particles tension",
        ["--alg", "solve([T-16/13*m*g-10/13*m*g=2*m*a,3*m*g-T=3*m*a],[T,a])"],
        ["T = 12*m*g/5", "a = g/5"],
        ["T ~= 0", "a ~= 0", "ERR:"],
    ),
    (
        "June 2019 Q3 post-ground rest check",
        ["--alg", "16/13*m*g-10/13*m*g"],
        ["6/13*m*g"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 drum reaction from moments",
        ["--alg", "solve(5*N=20*9.8*4*24/25,N,method=linear)"],
        ["N = 18816/125", "N ~= 150.528"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 ramp force components",
        ["--alg", "solve([F*24/25+R*7/25=20*9.8*7/25,150.528+R*24/25=20*9.8*24/25+F*7/25],[F,R])"],
        ["F = 131712/3125", "R = 160916/3125", "F ~= 42.14784", "R ~= 51.49312"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 resultant force at A",
        ["--alg", "sqrt(42.14784^2+51.49312^2),method=numeric"],
        ["66.543082"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 projectile vertical velocity",
        ["--suvat", "u=10,a=-9.8,t=2,target=v"],
        ["v = u + at", "v = -48/5", "v = -9.6 m/s"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 projectile speed",
        ["--alg", "sqrt((10*sqrt(3))^2+(-9.6)^2),method=numeric"],
        ["19.803030"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 projectile direction",
        ["--alg", "atan(9.6/(10*sqrt(3)))*180/pi,method=numeric"],
        ["28.997686"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 collision angle",
        ["--alg", "atan(10/(25-10*sqrt(3)))*180/pi,method=numeric"],
        ["52.477568"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 collision speed",
        ["--alg", "sqrt((25-10*sqrt(3))^2+10^2),method=numeric"],
        ["12.608512"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 speed",
        ["--alg", "sqrt(216)"],
        ["sqrt(216)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 acceleration i component",
        ["--derive", "3*t^2,t"],
        ["f' = 2*t", "dy/dt = 6*t"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 acceleration j component",
        ["--derive", "-6*sqrt(t),t"],
        ["f' = 1/(2*sqrt(t))", "dy/dt = -3/sqrt(t)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 position i primitive",
        ["--int", "int(3*t^2,t)"],
        ["I = Int(3*t^2) dt", "t^3 + C"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 position j primitive",
        ["--int", "int(-6*sqrt(t),t)"],
        ["I = Int(-6*sqrt(t)) dt", "-4*t^(3/2) + C"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 position vector at t=1",
        ["--alg", "1-64+1"],
        ["-62"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 position vector at t=1 j component",
        ["--alg", "-4+32-4"],
        ["24"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 friction from vertical resolve",
        ["--alg", "solve(F*3/5+68.6*4/5=5*9.8,F,method=linear)"],
        ["3*F/5 + 1372/25 = 49", "F = -49/5", "F ~= -9.800"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 acceleration down plane",
        ["--alg", "solve(5*9.8*3/5-0.5*(5*9.8*4/5)=5*a,a,method=linear)"],
        ["49/5 = 5*a", "a = 49/25", "a ~= 1.960"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 direction condition",
        ["--alg", "solve(lambda-3*mu+7=0,lambda,method=linear)"],
        ["lambda - 3*mu + 7 = 0", "lambda = 3*mu - 7"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 mu from lambda",
        ["--alg", "solve(2-3*mu+7=0,mu,method=linear)"],
        ["- 3*mu + 9 = 0", "mu = 3"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 displacement components",
        ["--alg", "1/2*(6/4)*4^2"],
        ["12"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 length AB",
        ["--alg", "sqrt(12^2+4^2)"],
        ["sqrt(160)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 moment equation",
        ["--alg", "solve(T*2*a=M*g*a*cos(theta)+2*M*g*(3*a/2)*cos(theta),T,method=linear)"],
        ["2*T*a = 4*M*g*a*cos(theta)", "T = 2*M*g*cos(theta)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 vertical reaction",
        ["--alg", "solve(R+2*M*g*(3/5)^2=3*M*g,R,method=linear)"],
        ["R + 18/25*M*g = 3*M*g", "R = 57/25*M*g"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 limiting friction coefficient",
        ["--alg", "solve(mu*(57/25*M*g)=2*M*g*(3/5)*(4/5),mu,method=linear)"],
        ["57/25*mu*M*g = 24/25*M*g", "mu = 8/19"],
        ["ERR:"],
    ),
    (
        "June 2022 Q5 horizontal projectile time",
        ["--suvat", "s=120,u=U*cos(alpha),v=,a=0,target=t"],
        ["s = ut", "t = 120/(U*cos(alpha))"],
        ["ERR:"],
    ),
    (
        "June 2022 Q5 vertical projectile equation",
        ["--suvat", "s=0,u=U*sin(alpha),a=-9.8,t=t,target=s"],
        ["s = ut + 1/2at^2", "s = U*sin(alpha)*t - 49/10*t^2"],
        ["ERR:"],
    ),
    (
        "June 2022 Q5 range constant",
        ["--alg", "9.8*120/2"],
        ["588"],
        ["ERR:"],
    ),
    (
        "June 2022 Q5 maximum height speed components",
        ["--alg", "14^2+42^2,method=numeric"],
        ["1960"],
        ["ERR:"],
    ),
    (
        "June 2023 Q1 speed",
        ["--suvat", "u=0,a=3.2,t=5,target=v"],
        ["v = u + at", "v = 16"],
        ["ERR:"],
    ),
    (
        "June 2023 Q1 distance",
        ["--suvat", "s=,u=0,a=3.2,t=5,target=s"],
        ["s = ut + 1/2at^2", "s = 40"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 normal reaction",
        ["--alg", "solve(R=5*9.8,R,method=linear)"],
        ["R = 49"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 friction equation",
        ["--alg", "solve(28-F=5*1.4,F,method=linear)"],
        ["-F = -21", "F = 21"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 coefficient of friction",
        ["--alg", "solve(mu*49=21,mu,method=linear)"],
        ["49*mu = 21", "mu = 3/7", "mu ~= 0.429"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 initial speed",
        ["--alg", "sqrt(7^2+(-3)^2)"],
        ["sqrt(58)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 parallel direction time",
        ["--alg", "solve(t^2-3*t+7=2*t^2-3,t,0,inf)"],
        ["Interval: t in [0, inf]", "t = [2]"],
        ["t = [-5", "ERR:"],
    ),
    (
        "June 2023 Q3 acceleration i component",
        ["--derive", "t^2-3*t+7,t"],
        ["d/dt(t^2) = 2*t", "dy/dt = 2*t - 3"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 acceleration j component",
        ["--derive", "2*t^2-3,t"],
        ["d/dt(2*t^2) = 4*t", "dy/dt = 4*t"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 acceleration perpendicular to i",
        ["--alg", "solve(2*t-3=0,t,method=linear)"],
        ["2*t = 3", "t = 3/2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 velocity i at B",
        ["--suvat", "u=-16,a=2.4,t=5,target=v"],
        ["v = u + at", "v = -4"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 velocity j at B",
        ["--suvat", "u=-3,a=1,t=5,target=v"],
        ["v = u + at", "v = 2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 speed at B",
        ["--alg", "sqrt((-4)^2+2^2)"],
        ["sqrt(20)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 time at C",
        ["--alg", "solve(4=-16*T+1/2*2.4*T^2+44,T,5,inf)"],
        ["Interval: T in [5, inf]", "T = [10]"],
        ["T = [10/3", "ERR:"],
    ),
    (
        "June 2023 Q4 c coordinate",
        ["--alg", "-3*10+1/2*1*10^2-10"],
        ["10"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 horizontal projectile time",
        ["--alg", "solve(28*cos(a)*T=40,T,method=linear)"],
        ["28*cos(a)*T = 40", "T = 10/(7*cos(a))"],
        ["T = 40/(28*cos(a))", "ERR:"],
    ),
    (
        "June 2023 Q5 tan quadratic",
        ["--alg", "solve(20=40*y-10*(1+y^2),y)"],
        ["10*y^2 - 40*y + 30 = 0", "y = [3, 1]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 maximum height",
        ["--suvat", "u=28*3/sqrt(10),v=0,a=-9.8,target=s"],
        ["v^2 = u^2 + 2as", "s = (v^2-u^2)/(2a)", "s = 36 m"],
        ["= v^2 = u^2 + 2as", "ERR:"],
    ),
    (
        "June 2023 Q6 moment equation",
        ["--alg", "solve(S*2*a*sin(theta)=M*g*a*cos(theta),S,method=linear)"],
        ["2*S*a*sin(theta) = M*g*a*cos(theta)", "S = M*g*cos(theta)/(2*sin(theta))"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 coefficient of friction",
        ["--alg", "solve((1/2)*M*g*(4/3)=mu*M*g,mu,method=linear)"],
        ["2/3*M*g = mu*M*g", "mu = 2/3"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 resultant at A",
        ["--alg", "sqrt((2/3)^2+1^2)*M*g"],
        ["sqrt(13/9)*M*g"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 normal reaction",
        ["--alg", "solve(R=0.5*9.8,R,method=linear)"],
        ["R = 49/10", "R = [4.9]", "R ~= 4.900"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 limiting friction force",
        ["--alg", "solve(X=(2/7)*4.9,X,method=linear)"],
        ["X = 7/5", "X = [1.4]", "X ~= 1.400"],
        ["ERR:"],
    ),
    (
        "June 2024 Q2 first acceleration",
        ["--suvat", "u=0,v=10,t=4,target=a"],
        ["a = (v-u)/t", "a = 5/2", "a = 2.5 m/s^2"],
        ["ERR:"],
    ),
    (
        "June 2024 Q2 first 18 seconds distance",
        ["--alg", "1/2*4*10+14*10"],
        ["160"],
        ["ERR:"],
    ),
    (
        "June 2024 Q2 finish speed",
        ["--alg", "solve(1/2*6*(10+U)=200-160,U,method=linear)"],
        ["3*(U + 10) = 40", "U = 10/3", "U ~= 3.333"],
        ["ERR:"],
    ),
    (
        "June 2024 Q3 inclined plane reaction",
        ["--alg", "solve(R=m*g*12/13,R,method=linear)"],
        ["R = 12*m*g/13"],
        ["ERR:"],
    ),
    (
        "June 2024 Q3 inclined plane acceleration",
        ["--alg", "solve(m*g*5/13-mu*(12/13*m*g)=m*a,a,method=linear)"],
        ["5*m*g/13 - 12/13*mu*m*g = m*a", "a = (5*m*g/13 - 12/13*mu*m*g)/m"],
        ["ERR:"],
    ),
    (
        "June 2024 Q3 acceleration equivalent form",
        ["--alg", "compare((5*m*g/13 - 12/13*mu*m*g)/m,g*(5-12*mu)/13)"],
        ["E1-E2 = 0", "equivalent"],
        ["ERR:", "not equivalent"],
    ),
    (
        "June 2024 Q4 bearing constant",
        ["--alg", "solve(2*c=6,c,method=linear)"],
        ["2*c = 6", "c = 3", "c ~= 3.000"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 velocity i component",
        ["--derive", "3*sqrt(t),t"],
        ["f' = 1/(2*sqrt(t))", "dy/dt = 3/(2*sqrt(t))"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 velocity j component",
        ["--derive", "-3/8*t^2,t"],
        ["f' = 2*t", "dy/dt = -3/4*t"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 speed at t=4",
        ["--alg", "sqrt((3/4)^2+(-3)^2)"],
        ["sqrt(153/16)"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 acceleration i component",
        ["--derive", "3/(2*sqrt(t)),t"],
        ["y' = (u'v-u*v')/v^2", "dy/dt = -3/sqrt(t)/(2*sqrt(t))^2"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 acceleration j component",
        ["--derive", "-3/4*t,t"],
        ["f' = 1", "dy/dt = -3/4"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 acceleration direction time",
        ["--alg", "solve(T^(3/2)=27,T,1,inf)"],
        ["Interval: T in [1, inf]", "T = [9]", "T ~= 9.000"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 projectile horizontal equation",
        ["--suvat", "s=x,u=35*(4/5),a=0,t=t,target=s"],
        ["s = ut + 1/2at^2", "s = 28*t"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 projectile vertical equation",
        ["--suvat", "s=y,u=35*(3/5),a=-9.8,t=t,target=s"],
        ["s = ut + 1/2at^2", "s = 21*t - 49/10*t^2"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 path equation equivalence",
        ["--alg", "compare(21*(x/28)-49/10*(x/28)^2,3*x/4-x^2/160)"],
        ["E1-E2 = 0", "equivalent"],
        ["ERR:", "not equivalent"],
    ),
    (
        "June 2024 Q5 range from path",
        ["--alg", "solve(3/4*x-1/160*x^2=0,x,0,inf)"],
        ["x = 0 or x = 120", "x = [0, 120]"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 stationary derivative",
        ["--derive", "3/4*x-1/160*x^2,x"],
        ["d/dx(3/4*x) = 3/4", "dy/dx = - 1/80*x + 3/4"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 maximum height x",
        ["--alg", "solve(3/4-x/80=0,x,method=linear)"],
        ["-x = -60", "x = [60]"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 maximum height",
        ["--alg", "evalat(3/4*x-1/160*x^2,x,60)"],
        ["f(60) = 45/2"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 rod moment equation",
        ["--alg", "solve(S*1.5*a=M*g*a*(3/5),S,method=linear)"],
        ["3/2*S*a = 3/5*M*g*a", "S = 2*M*g/5"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 horizontal reaction equation",
        ["--alg", "solve(F=(2/5*M*g)*(4/5),F,method=linear)"],
        ["F = 8/25*M*g"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 vertical reaction equation",
        ["--alg", "solve(R=M*g-(2/5*M*g)*(3/5),R,method=linear)"],
        ["R = 19/25*M*g"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 limiting friction coefficient",
        ["--alg", "solve(mu*(19/25*M*g)=8/25*M*g,mu,method=linear)"],
        ["19/25*mu*M*g = 8/25*M*g", "mu = 8/19"],
        ["ERR:"],
    ),
]


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [n for n in needles if not markers_present(out, [n])]
    bad = [b for b in banned if b in out]
    if proc.returncode:
        misses.append(f"returncode={proc.returncode}")
    if misses or bad:
        return [f"{name}: missing={misses} banned={bad}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles, banned in CASES:
        failures.extend(run_case(name, args, needles, banned))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"edexcel_paper32_downloads ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
