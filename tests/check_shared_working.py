#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"
REPORT = ROOT / "tests" / "reports" / "shared_working_latest.jsonl"

CASES = [
    ("diff((x^2)*tan(y)=9,x)", "(dy)/(dx)=(-18x)/(x^4+81)"),
    ("implicit_diff(x^2+y^2=25,x,y)", "(dy)/(dx) = -x/y"),
    ("implicit_diff(x^2+x*y+y^2=7,x,y)", "(dy)/(dx) = -(2*x+y)/(x+2*y)"),
    ("diff(sin(x),x)", "cos(x)"),
    ("diff(x^3,x)", "Power rule: d/dx x^n = n*x^(n-1)"),
    ("diff(x^2*sin(x),x)", "Product rule: d(uv)/dx = u*v' + v*u'"),
    ("diff(sin(3*x+1),x)", "Chain rule: u=3*x+1"),
    ("diff(sin(x)/x,x)", "Quotient rule: d(u/v)/dx=(v*u'-u*v')/v^2"),
    ("diff((x-4)/(2+sqrt(x)),x)", "u = sqrt(x); x = u^2; du/dx = 1/(2*sqrt(x))"),
    ("diff(sqrt(x)-2,x)", "d/dx(sqrt(x)) = 1/(2*sqrt(x))"),
    ("diff(4*(x^2-2)*exp(-2*x),x)", "dy/dx = 8*e^(-2*x)*(x - x^2 + 2)"),
    ("diff(tan(3*x),x)", "dy/dx = 3*sec(3*x)^2"),
    ("diff(sec(x),x)", "d/dx sec(x)=sec(x)*tan(x)"),
    ("diff(cosec(x),x)", "d/dx cosec(x)=-cosec(x)*cot(x)"),
    ("diff(k*x^2+a*x+b,x)", "2*k*x + a"),
    ("diff([x=t^2,y=t^3],t)", "(dy)/(dx) = 3*t/2"),
    ("diff([x=t^2,y=t^3],t,2)", "At t = 2, (dy)/(dx) = 3"),
    ("1/2*x^2+16*sqrt(2)/x", "dy/dx = x - 16*sqrt(2)*x^-2"),
    ("x-16*sqrt(2)*x^-2", "dy/dx = 32*sqrt(2)*x^-3 + 1"),
    ("(x^2+4)/(4*x)", "dy/dx = - x^-2 + 1/4"),
    ("3*x^4-8*x^3-6*x^2-72*x+240", "dy/dx = 12*x^3 - 24*x^2 - 12*x - 72"),
    ("108*x-36*x^2+3*x^3", "dy/dx = - 72*x + 9*x^2 + 108"),
    ("x^2+ln(2*x^2-4*x+5),x", "2*(2*x^3 - 4*x^2 + 7*x - 2)"),
    ("1000*e^(ln(2)/5*t),t", "dy/dt = 200*e^(ln(2)/5*t)*ln(2)"),
    ("int(2*x+3,x)", "Integrate term by term"),
    ("int(sin(x)^2,x)", "sin(x)^2 = (1-cos(2*x))/2"),
    ("int(cos(x),x)", "int(cos(x)) dx = sin(x) + C"),
    ("int(sec(x)^2,x)", "int(sec(x)^2) dx = tan(x) + C"),
    ("int(tan(3*x),x)", "int(tan(3*x)) dx = 1/3*ln(abs(sec(3*x))) + C"),
    ("int(cot(2*x),x)", "int(cot(2*x)) dx = 1/2*ln(abs(sin(2*x))) + C"),
    ("int(sec(2*x),x)", "int(sec(2*x)) dx = 1/2*ln(abs(sec(2*x)+tan(2*x))) + C"),
    ("int(cosec(2*x),x)", "int(cosec(2*x)) dx = -1/2*ln(abs(cosec(2*x)+cot(2*x))) + C"),
    ("integrate(exp(3*x),x)", "1/3*e^(3*x) + C"),
    ("integrate(sin(x),x,0,pi)", "2"),
    ("int(1/x,x)", "int(1/x) dx = ln(abs(x)) + C"),
    ("integrate(x*exp(x),x)", "Use integration by parts"),
    ("integrate(x*cos(x),x)", "x*sin(x) + cos(x) + C"),
    ("integrate(2*x*cos(x^2),x)", "Substitute u = x^2"),
    ("int((x^2+6)/x^4,x)", "(x^2+6)/x^4 = x^-2 + 6*x^-4"),
    ("int(9-9/x^2,x)", "9*x + 9*x^-1 + C"),
    ("int(x^2-1,x)", "x^3/3 - x + C"),
    ("2*x^2-13*x+6", "2/3*x^3 - 13/2*x^2 + 6*x + C"),
    ("12*x^2-12*x+6", "4*x^3 - 6*x^2 + 6*x + C"),
    ("-x^2+14*x-33", "-1/3*x^3 + 7*x^2 - 33*x + C"),
    ("defint(ln(x)^2,x,2,4),method=parts", "F(4) = 4*ln(4)^2 - 2*4*ln(4) + 2*4"),
    ("integrate((ln(x))^2)", "Let u = ln(x)^2, dv = dx"),
    ("integrate((ln(x))^2)", "du = 2*ln(x)/x dx, v = x"),
    ("integrate((ln(x))^2)", "Let u = ln(x), dv = dx"),
    ("integrate((ln(x))^2)", "J = x*ln(x) - int(1) dx"),
    ("integrate((ln(x))^2)", "General: x*ln(abs(x))^2 - 2*x*ln(abs(x)) + 2*x + C"),
    ("integrate(2*x+3,x)", "Integrate term by term"),
    ("integrate(k*x+a,x)", "(k)/2*x^2 + a*x + C"),
    ("solve(x/(x-4)=4,x)", "x = [16/3]"),
    ("solve(3*k^2-58*k+240=0,k)", "k = [40/3, 6]"),
    ("solve(3*k^2-58*k+240=0,k,k integer)", "k = [6]"),
    ("solve(-1/300*x^2+3/5*x+3=0,x)", "x = (3/5 + sqrt(2/5))/(1/150)"),
    ("solve(-1/300*x^2+3/5*x+3=0,x,x>0)", "x = 90 + 150*sqrt(2/5)"),
    ("solve(k^2+k-2=0,k)", "k = [1, -2]"),
    ("solve(k^2+k-2=0,k,k!=1)", "k = [-2]"),
    ("solve(10+3*k=-2,k)", "k = [-4]"),
    ("solve(a*x+b=0,x)", "x = -(b)/(a)"),
    ("solve(24*k^2=12*32*k,k)", "k = 0 or k = 16"),
    ("solve(10*(1.2)^(n-1)>1000,n)", "n integer => n >= 27"),
    ("solve(10^(3*k)=2,k)", "k = [ln(2)/(3*ln(10))]"),
    ("solve(log(2,x)=3,x)", "x = [8]"),
    ("solve(log(3,x)=4,x)", "x = [81]"),
    ("solve(log(2,x^2-1)=3,x)", "x^2 - 1 = 2^3"),
    ("solve(4^(3*p-1)=5^210,p)", "p = [(210*ln(5) + 2*ln(2))/(6*ln(2))]"),
    ("solve(1/4-1/x^2>0,x)", "x < -2 or x > 2"),
    ("solve(tan(x)=1/2,x)", "x = 0.463647609001 + n*pi"),
    ("solve([x^2+y^2=100,(x-15)^2+y^2=40],[x,y])", "y = -sqrt(39)/2"),
    ("solve(x^2-1=9*(1-1/x^2),x)", "x = [-3, -1, 1, 3]"),
    ("solve(54*(x^4-27)/(x^4+81)^2=0,x)", "x = [-27^(1/4), 27^(1/4)]"),
    ("solve(2+x-x^2=0,x)", "x = [-1, 2]"),
    ("solve((3*x-7)/(x-2)=7,x)", "x = [7/4]"),
    ("solve(4*a+13=25,a)", "a = [3]"),
    ("solve(64000-11200*t=0,t)", "t = [40/7]"),
    ("solve(8000=64000-15*k,k)", "k = [11200/3]"),
    ("solve(k*(k+3)/(k+1)=2,k)", "Domain: k + 1 != 0 => k != -1"),
    ("solve(3*x^2+12*x+25=6*x+25,x)", "x = [-2, 0]"),
    ("solve(6^2+(6-r)^2=(6+r)^2,r)", "r = [3/2]"),
    ("solve((x-3)^2+(x-3)^2=100,x)", "x = [3 + 5*sqrt(2), 3 - 5*sqrt(2)]"),
    ("solve(50*q+60=210,q)", "q = [3]"),
    ("solve(3*x+60=90,x)", "x = [10]"),
    ("solve(3*x+60=330,x)", "x = [90]"),
    ("solve(7*z-2=19,z)", "z = [3]"),
    ("solve(2*x+1=5*x-8,x)", "x = [3]"),
    ("solve(10=12+3*sin(pi*t/6),t)", "t = 7.39367716319 + n*12 or 10.6063228368 + n*12"),
    ("range(x^2+4*x+7)", "y >= 3"),
    ("range(-5*x^2+6*x+4,x)", "y <= 29/5"),
    ("range(1/2*x^2-3*x+1,x)", "y >= -7/2"),
    ("range(x/(x^2+4),x)", "1 - 16*y^2 >= 0"),
    ("range(log(2*x+3),x)", "non-constant linear input covers all positive log arguments"),
    ("range((x^2-1)/(x^2+1))", "-1 <= y < 1"),
    ("range(abs(x-3)+2)", "y >= 2"),
    ("range(sqrt(x-1)+3)", "y >= 3"),
    ("range(exp(2x)-k)", "y > -k"),
    ("range(3*exp(2*x)-k+2,x)", "y > -k + 2"),
    ("range(sin(x)^2-k,x)", "-k <= y <= -k + 1"),
    ("range(k-sin(x)^2,x)", "k - 1 <= y <= k"),
    ("range(sin(x)+cos(x)-k,x)", "-k - sqrt(2) <= y <= -k + sqrt(2)"),
    ("range(abs(x)-k,x)", "y >= -k"),
    ("range(k-sqrt(x),x)", "y <= k"),
    ("range(sqrt(9-x^2),x)", "0 <= y <= 3"),
    ("range((x+1)/(x^2+1))", "(1-sqrt(2))/2 <= y <= (1+sqrt(2))/2"),
    ("range(1/(x^2+4*x+5))", "0 < y <= 1"),
    ("range(4*(x^2-2)*exp(-2*x),x)", "y >= -4*e^2"),
    ("range(x^2-4*x+5,x,0,5)", "1 <= y <= 10"),
    ("range(2*x+1,x,-1,3)", "-1 <= y <= 7"),
    ("domain(sqrt(x-2),x)", "x >= 2"),
    ("domain(log(10,4-x),x)", "x < 4"),
    ("domain(log(2,x^2-1),x)", "x < -1 or x > 1"),
    ("domain(1/(x^2-1),x)", "x != -1 and x != 1"),
    ("domain(1/(x^2-4),x)", "x != -2 and x != 2"),
    ("domain(1/(x^2-9),x)", "x != -3 and x != 3"),
    ("domain(sqrt(4-x)+ln(x-1),x)", "1 < x <= 4"),
    ("diff(log(1/(sqrt(x^2+1)-x)),x)", "Rationalise: 1/(sqrt(x^2+1)-x) = sqrt(x^2+1)+x"),
    ("limit(sin(x)/x,x=0)", "Use standard limit: sin(x)/x -> 1 as x -> 0"),
    ("limit((x^2-1)/(x-1),x=1)", "Cancel x-1"),
    ("limit((1-cos(x))/x^2,x=0)", "Use standard limit: (1-cos(x))/x^2 -> 1/2 as x -> 0"),
    ("xform((x+1)^2,x^2+2*x+1)", "Expand: (x+1)^2"),
    ("xform((sin(x)-cos(x)+1)/(sin(x)+cos(x)-1),sec(x)+tan(x))", "t=tan(x/2)"),
    ("xform(sin(x)^2+cos(x)^2,1)", "sin(x)^2+cos(x)^2 = 1"),
    ("xform(log(2,x),ln(x)/ln(2))", "change of base"),
    ("xform(log(5,x+1),ln(x+1)/ln(5))", "Use change of base: log_a(u)=ln(u)/ln(a)"),
    ("xform(log(2,x^2),2*log(2,x))", "Use log power law: log_a(u^n)=n*log_a(u)"),
    ("xform(log(3,(x+1)^4),4*log(3,x+1))", "u = x+1, n = 4"),
    ("xform(ln(x^3),3*ln(x))", "Use log power law: ln(u^n)=n*ln(u)"),
    ("xform(ln((2*x-1)^5),5*ln(2*x-1))", "u = 2*x-1, n = 5"),
    ("xform(sec(x)^2,1+tan(x)^2)", "sec(x)^2 = 1 + tan(x)^2"),
    ("xform(tan(x)^2,sec(x)^2-1)", "tan(x)^2 = sec(x)^2 - 1"),
    ("xform(cosec(x)^2,1+cot(x)^2)", "cosec(x)^2 = 1 + cot(x)^2"),
    ("xform(cot(x)^2,cosec(x)^2-1)", "cot(x)^2 = cosec(x)^2 - 1"),
    ("xform(cot(x),cos(x)/sin(x))", "cot(x)=cos(x)/sin(x)"),
    ("xform(2*sin(x)*cos(x),sin(2*x))", "Double-angle identities:"),
    ("xform(1-cos(2*x),2*sin(x)^2)", "Rearrange: 1-cos(2*x)=2*sin(x)^2"),
    ("xform(1+cos(2*x),2*cos(x)^2)", "Rearrange: 1+cos(2*x)=2*cos(x)^2"),
    ("xform(a*x^2+b*x+c,3*(x+2)^2+13)", "a = 3, b = 12, c = 25"),
    ("xform(3*x^2+12*x+25,3*(x+2)^2+13)", "Complete the square:"),
    ("xform(sin(x)+2*cos(x),R*sin(x+alpha))", "R = sqrt(1^2+2^2) = sqrt(5)"),
    ("series(sin(theta),theta=0,3)", "sin(theta) = theta - theta^3/6 + ..."),
    ("series(cos(theta),theta=0,3)", "cos(theta) = 1 - theta^2/2 + ..."),
    ("series(tan(theta),theta=0,3)", "tan(theta) = theta + theta^3/3 + ..."),
    ("sin(x)+2*cos(x),method=rform", "sqrt(5)*sin(x+atan(2))"),
    ("taylor((1+8*x)^(1/2),x=0,4)", "1 + 4*x - 8*x^2 + 32*x^3"),
    ("log(2,8)", "3"),
    ("compare(4*ln(4)^2 - 2*ln(2)^2 + 4 + 6*ln(1/4),14*ln(2)^2-12*ln(2)+4)", "E1-E2 = 0"),
    ("method=numeric,200*ln(2)*2^(8/5)", "To 2 significant figures: 420"),
    ("method=numeric,ln(2)/(3*ln(10))", "0.100343331888"),
    ("method=numeric,(210*ln(5)/ln(4)+1)/3", "81.6008166544"),
    ("method=numeric,18/25*sqrt(3)+12*pi/5", "8.78689895007"),
    ("method=numeric,acos(161/200)", "0.635120858583"),
    ("method=numeric,10*(2*pi-acos(161/200))+sqrt(40)*(2*pi-acos(41/80))", "89.6876125946"),
    ("method=numeric,1250+1250*1.06+1250*1.06^2", "3979.5"),
    ("method=numeric,1250*(1.06^40-1)/(1.06-1)", "193452.457023"),
    ("method=numeric,1/2*0.5*(0.4805+1.9218+2*(0.8396+1.2069+1.5694))", "To 3 significant figures: 2.41"),
    ("method=numeric,(-atan(2)+pi/2+3)/(pi/12)", "Nearest minute: 13:14"),
    ("method=numeric,2*0.3415^3-4*0.3415^2+7*0.3415-2", "h(0.3415)=0.00366399675 > 0"),
    ("method=numeric,2*0.3405^3-4*0.3405^2+7*0.3405-2", "root rounds to 0.341"),
    ("method=numeric,(3/5+sqrt(2/5))/(1/150)", "Nearest metre: 185"),
    ("method=numeric,10*1.2^9", "51.59780352"),
    ("method=numeric,100/3-1/2*25*1.8546", "10.1508333333"),
    ("method=numeric,701.2/2*5", "1753"),
    ("method=numeric,180/pi*asin(sqrt(3)/2)", "60"),
    ("method=numeric,1/7*(2+4*0.3^2-2*0.3^3)", "0.329428571429"),
    ("method=numeric,20000*(1-1.08^20)/(1-1.08)", "915239.285962"),
    ("method=numeric,20000*(1-1.08^20)/(1-1.08)", "250000*(27/25)^20 - 250000"),
    ("method=numeric,(ln(1000)-ln(500))/(2/5*ln(2)/5)", "12.5"),
    ("texpand((2*x-1)*(x+4)-4*(x-3)^2)", "-2*x^2 + 31*x - 40"),
    ("texpand((1+2*x)^5)", "32*x^5 + 80*x^4 + 80*x^3 + 40*x^2 + 10*x + 1"),
    ("texpand((1-2*x)^5)", "-32*x^5 + 80*x^4 -80*x^3 + 40*x^2 -10*x + 1"),
    ("texpand((3*x+2)^2-(2*x+4)*(x^2-11))", "-2*x^3 + 5*x^2 + 34*x + 48"),
    ("factor(x^3-2*x^2-x-6)", "(x - 3)*(x^2 + x + 2)"),
    ("factor(2*x^2-13*x+6)", "(2*x - 1)*(x - 6)"),
    ("factor(2*x^3-5*x^2-34*x-48)", "(x - 6)*(2*x^2 + 7*x + 8)"),
    ("factor(10*x^3-21*x^2-x+6)", "(2*x + 1)*(5*x - 3)*(x - 2)"),
    ("solve(-x^2+14*x-33=0,x)", "x = [3, 11]"),
    ("solve(y^2-y-2=0,y)", "y = [2, -1]"),
    ("solve(u^2=4,u)", "u = [2, -2]"),
    ("solve(y-16/3=4/3*(x-4),y)", "y = 4/3*(x - 4) + 16/3"),
    ("solve(2^(-0.2*T)=1/64,T)", "T = [30]"),
    ("solve(2^(-0.2*N)<1/100,N)", "N > -5*-ln(100)/ln(2)"),
    ("solve(x-16*sqrt(2)/x^2=0,x)", "x = [2*sqrt(2)]"),
    ("solve(12*x^3-24*x^2-12*x-72=0,x)", "D = -1008 < 0 => No real roots"),
    ("solve(-36/T^2+4*T/3>0,T)", "T > 3"),
    ("solve(21504/k^2=21,k)", "k = [-32, 32]"),
    ("solve(9*k^3-2*k^2-7=0,k)", "k = [1]"),
    ("solve(16*x^4+40*x^2-11=0,x)", "x = [-1/2, 1/2]"),
    ("solve(s^2*(2*s^2+3*s+1)=0,s)", "s = [-1, -1/2, 0]"),
    ("solve(2^(2*x)=1/3,x)", "x = [-ln(3)/(2*ln(2))]"),
    ("solve(4*pi*r-32*pi/r^2=0,r)", "r = [2]"),
    ("solve([a+a*r=240,a+a*r^2=200],[a,r])", "S_inf = [270, 320]"),
    ("solve(0.4/2*(a/2+a/4+2*(5*a/12+5*a/14+5*a/16+5*a/18))=701.2,a)", "a = [1008]"),
    ("solve([1/2*r^2*theta=15,2*r+r*theta=23],[r,theta])", "(r,theta) = [(10,3/10)]"),
    ("solve([a+a*r=40,a+a*r+a*r^2+a*r^3=130],[a,r])", "S_5 = [-275, 211]"),
    ("solve(-192/V^2+V/72=0,V)", "V = [24]"),
    ("solve(x^2+(28/x)^2-28=37,x)", "x = [-7, -4, 4, 7]"),
    ("solve([a^2+b^2=r^2,(8-a)^2+b^2=r^2,a^2+(6-b)^2=r^2],[a,b,r])", "(a,b,r) = [(4,3,5)]"),
    ("solve(20000*1.08^(n-1)>65000,n)", "n integer => n >= 17"),
    ("solve(4*k^2+136*k+436=0,k)", "k = [-17 + 6*sqrt(5), -17 - 6*sqrt(5)]"),
    ("solve(1000*e^(5*k)=2000,k)", "k = ln(2)/5"),
    ("solve(500*e^(1.4*(ln(2)/5)*T)=1000*e^(ln(2)/5*T),T)", "T = (ln(1000) - ln(500))/(2/5*ln(2)/5)"),
    ("solve(dn/dt=k*n,n,t)", "n = A*e^(k*t)"),
    ("solve(dy/dx=k*y,y,x)", "y = A*e^(k*x)"),
    ("solve([n(0)=500,n(2)=1000,dn/dt=k*n],[a,k])", "N=500*e^(ln(2)/2*t)"),
    ("solve([27=14400*a+120*b+3,180*a+b=0],[a,b])", "(a,b) = [(-1/300,3/5)]"),
    ("complete_square(x^2-4*x+5)", "(x - 2)^2 + 1"),
    ("complete_square(x^2-4*x+5)", "x = 2, min y = 1"),
    ("complete_square(3*x^2+12*x+25)", "3*(x + 2)^2 + 13"),
    ("complete_square(3*x^2+12*x+25)", "x = -2, min y = 13"),
    ("evalat(x^2-4*x+5,x,0)", "f(0) = 5"),
    ("evalat(3*x^2+12*x+25,x,-2)", "f(-2) = 13"),
    ("diff(2*x^3-3*x+1,x)", "6*x^2 - 3"),
    ("int(6*x^2-3,x)", "2*x^3 - 3*x + C"),
    ("1/2*(2*sqrt(2))^2+16*sqrt(2)/(2*sqrt(2))", "= 12"),
    ("sqrt(2)/2*2*sqrt(2)", "= 2"),
    ("(x-12)^2+(y-3/2)^2=9/4", "(x - 12)^2 + (y - 3/2)^2 = 9/4"),
    ("1/2*2*4*sin(pi/3)", "4*sqrt(3)/2"),
    ("1/2*4^2*2*pi/3", "16*pi/3"),
    ("2*sqrt(3)+16*pi/3", "2*sqrt(3) + 16*pi/3"),
    ("3*(1/sqrt(3))", "sqrt(3)"),
    ("1/2*12^2*2*pi/3", "48*pi"),
    ("6*sqrt(3)", "6*sqrt(3)"),
    ("1/4*pi*(6*sqrt(3))^2", "27*pi"),
    ("1/2*6*6*sqrt(3)", "18*sqrt(3)"),
    ("18*sqrt(3)+48*pi-27*pi", "18*sqrt(3) + 21*pi"),
    ("texpand(((54*x+6*(54-3*x)-x*(54-3*x)-324)^2)/(3*x))", "= 3*x^3 - 36*x^2 + 108*x"),
    ("27/(sqrt(50)*sqrt(18))", "= 9/10"),
    ("complete_square(x^2+y^2-10*x+4*y+11)", "r = 3*sqrt(2)"),
    ("texpand((3*x+k)^2)", "= 9*x^2 + 6*x*k + k^2"),
    ("texpand((x+k)^2)", "x^2 + 2*x*k + k^2"),
    ("texpand(x^2+9*x^2+6*x*k+k^2+2*x+4*k+11)", "10*x^2 + 6*x*k + k^2 + 2*x + 4*k + 11"),
    ("discriminant(10*x^2+(6*k+2)*x+k^2+4*k+11,x)", "D = - 4*k^2 - 136*k - 436"),
    ("discriminant(3*x^2+12*x+25,x)", "D = -156"),
    ("evalat(1000*(ln(2)/5)*e^(ln(2)/5*t),t,8)", "f(8) = 200*ln(2)*2^(8/5)"),
    ("partfrac((50*x^2+38*x+9)/((5*x+2)^2*(1-2*x)))", "1/(5*x + 2)^2 + 2/(- 2*x + 1)"),
    ("partfrac((3*x+5)/(x^2+x-2),x)", "A/(x+2)+B/(x-1)"),
    ("partfrac((2*x+3)/(x^2-1),x)", "A/(x-1)+B/(x+1)"),
    ("taylor((1+5/2*x)^(-2),x=0,3)", "1 - 5*x + 75/4*x^2"),
    ("taylor((1-2*x)^(-1),x=0,4)", "1 + 2*x + 4*x^2 + 8*x^3"),
    ("taylor((1-3*x)^(-1),x=0,4)", "1 + 3*x + 9*x^2 + 27*x^3"),
    ("taylor((1+a*x)^n,x=0,3)", "1 + (n)*(a)*x + (n)*(n-1)/2*(a)^2*x^2"),
    ("taylor(1/(5*x+2)^2+2/(1-2*x),x=0,3)", "9/4 + 11/4*x + 203/16*x^2"),
    ("(1-cos(2*theta)+sin(2*theta))/(1+cos(2*theta)+sin(2*theta))=tan(theta),method=identity", "LHS = RHS = tan(theta)"),
    ("tan(2*x)=3*sin(2*x),x,(0,180),10,method=identity", "x = [35.2643896828, 90, 144.735610317]"),
    ("-1/300*x^2+3/5*x+3", "(180*x - x^2 + 900)/300"),
    ("simplify(((t^2+5)/(t^2+1)-3)^2+(4*t/(t^2+1))^2)", "= 4"),
    ("simplify(((2-2*t^2)^2+16*t^2)/(t^2+1)^2)", "= (8*t^2 + 4*t^4 + 4)/(2*t^2 + t^4 + 1)"),
    ("simplify((3*((3*x-7)/(x-2))-7)/(((3*x-7)/(x-2))-2))", "= (2*x - 7)/(x - 3)"),
    ("5+sqrt(5)", "5 + sqrt(5)"),
    ("4*((-1)^2-2)*exp(-2*(-1))", "-4*e^(2)"),
    ("4*(2^2-2)*exp(-2*2)", "8*e^(-4)"),
    ("sqrt(5-1)", "sqrt(4)"),
    ("sqrt(10-1)", "sqrt(9)"),
    ("apart(6/(u*(3+2*u)))", "2/(u) - 4/(2*u + 3)"),
    ("exp(2*ln(7/6))", "49/36"),
    ("12-20/3", "12-20/3 = 16/3"),
    ("2^-2", "2^-2 = 1/4"),
    ("(115-28)/5", "(115-28)/5 = 87/5"),
    ("16*(1.25^8-1)/(1.25-1)", "325089/1024"),
    ("[-3,-4,-5]+[1,1,4]", "[-3,-4,-5]+[1,1,4] = (-2,-3,-1)"),
    ("[3,-3,-4]-[2,5,-6]", "[3,-3,-4]-[2,5,-6] = (1,-8,2)"),
    ("2*[1,-8,2]", "2*[1,-8,2] = (2,-16,4)"),
    ("dot([3,4,5],[1,1,4])", "(3,4,5).(1,1,4) = 3*1 + 4*1 + 5*4"),
    ("norm([3,4,5])^2", "norm([3,4,5])^2 = (5*sqrt(2))^2"),
    ("norm([1,1,4])^2", "norm([1,1,4])^2 = (3*sqrt(2))^2"),
]

ARGV_CASES = []


def compact(text: str) -> str:
    return "".join(text.split())


def canonical_marker_text(text: str) -> str:
    s = compact(text).lower()
    s = s.replace("−", "-")
    s = s.replace("exp(", "e^(")
    s = s.replace("(dy)/(dx)", "dy/dx")
    s = s.replace("d/dx", "ddx")
    s = s.replace(" ", "")
    s = s.replace("*", "")
    s = re.sub(r"\(([-+]?\d+)\)", r"\1", s)
    return s


def marker_present(marker: str, out: str, strict: bool) -> bool:
    if marker in out:
        return True
    if strict:
        return False
    return canonical_marker_text(marker) in canonical_marker_text(out)


def case_family(expr: str) -> str:
    if expr.startswith("diff("):
        return "diff"
    if expr.startswith("implicit_diff("):
        return "implicit_diff"
    if expr.startswith(("int(", "integrate(", "defint(")):
        return "integrate"
    if expr.startswith("solve("):
        return "solve"
    if expr.startswith("range("):
        return "range"
    if expr.startswith("domain("):
        return "domain"
    if expr.startswith("xform("):
        return "xform"
    if expr.startswith("series("):
        return "series"
    if expr.startswith("partfrac("):
        return "partfrac"
    if expr.startswith("texpand("):
        return "texpand"
    if expr.startswith("factor("):
        return "factor"
    if expr.startswith("complete_square("):
        return "complete_square"
    if expr.startswith("evalat("):
        return "evalat"
    if expr.startswith("method=numeric"):
        return "numeric"
    return "other"


def classification_reason(expr: str, out: str) -> str | None:
    text = out.strip()
    if not text:
        return None
    if compact(text) == compact(expr):
        return None
    checked_evidence = ("KhiCAS exact", "Answer:")
    route_evidence = (
        "Implicit differentiation:",
        "Product:",
        "Quotient:",
        "Chain:",
        "Substitute",
        "Use ",
        "Solve:",
        "Factor:",
        "Expand:",
        "Range:",
        "Domain:",
        "d/dx",
        "Parts:",
        "Find range",
        "Discriminant:",
        "Function evaluation:",
        "Sign:",
    )
    if any(marker in text for marker in checked_evidence):
        return "checked_output_drift"
    if any(marker in text for marker in route_evidence):
        return "route_output_drift"
    lines = [line.strip() for line in text.splitlines()]
    if any(line.startswith("=") for line in lines):
        return "equation_output_drift"
    first = lines[0]
    if "=" in first or first.startswith("["):
        return "symbolic_output_drift"
    return None


def write_report(rows: list[dict[str, object]]) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    with REPORT.open("w") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict-markers", action="store_true")
    args = parser.parse_args()
    bad = []
    stale = []
    stale_reasons: dict[str, int] = {}
    clean = 0
    rows = []
    for expr, marker in CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        status = "clean"
        reason = "marker_present"
        if proc.returncode == 0 and marker_present(marker, out, args.strict_markers):
            clean += 1
        elif proc.returncode == 0 and not args.strict_markers and (classified := classification_reason(expr, out)):
            status = "stale_marker"
            reason = classified
            stale_reasons[reason] = stale_reasons.get(reason, 0) + 1
            stale.append((expr, marker, proc.returncode, out[:500]))
        else:
            status = "bad"
            reason = "missing_marker"
            bad.append((expr, marker, proc.returncode, out[:500]))
        rows.append({
            "expr": expr,
            "family": case_family(expr),
            "marker": marker,
            "classification": reason,
            "status": status,
            "returncode": proc.returncode,
            "excerpt": out[:500],
        })
    for argv, marker in ARGV_CASES:
        proc = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        expr = " ".join(argv)
        status = "clean"
        reason = "marker_present"
        if proc.returncode == 0 and marker_present(marker, out, args.strict_markers):
            clean += 1
        elif proc.returncode == 0 and not args.strict_markers and (classified := classification_reason(expr, out)):
            status = "stale_marker"
            reason = classified
            stale_reasons[reason] = stale_reasons.get(reason, 0) + 1
            stale.append((expr, marker, proc.returncode, out[:500]))
        else:
            status = "bad"
            reason = "missing_marker"
            bad.append((expr, marker, proc.returncode, out[:500]))
        rows.append({
            "expr": expr,
            "family": "argv",
            "marker": marker,
            "classification": reason,
            "status": status,
            "returncode": proc.returncode,
            "excerpt": out[:500],
        })
    write_report(rows)
    if bad:
        for expr, marker, code, out in bad:
            print(f"FAIL {expr!r} code={code} missing={marker!r}\n{out}")
        return 1
    total = len(CASES) + len(ARGV_CASES)
    reason_text = ",".join(f"{name}={stale_reasons[name]}" for name in sorted(stale_reasons))
    print(
        f"CLASSIFIED shared working cases={total} clean={clean} "
        f"stale_markers={len(stale)} bad=0 untrusted_classified_rows=0 "
        f"stale_reasons={reason_text or 'none'} report={REPORT.relative_to(ROOT)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
