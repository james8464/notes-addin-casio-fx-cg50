@t004
...
@END
@t005

@END
@t006
;t=
@END
@t009
u = f(x)
@END
@t010
du = u' dx
@END
@t011
Int 1 du
@END
@t012
x = ...
@END
@t013
I = u*v - Int(v du)
@END
@t015
du = u' dx; v = Int dv
@END
@t016
I = u*v - Int(v du)
@END
@t017
PF: A/(x-a)+(Bx+C)/q
@END
@t018
A/lin,B/lin2,(Cx+D)/q
@END
@t019
num = A*q+(Bx+C)(x-a)
@END
@t020
u = f(x)
@END
@t021
(uv)' = u'v+uv'
@END
@t022
(u/v)' = (u'v-uv')/v^2
@END
@t023
ln(y)=ln(f(x)); y'=y*d/dx(ln f)
@END
@t024
[cos(x+h)-cos(x)]/h
lim h->0 = -sin(x)
@END
@t025
[f(x+h)-f(x)]/h -> f'(x)
@END
@t026
dy/dx = d/dx(y)
@END
@t027
u = asin(x-1); 5u = pi
x = 1 + sin(pi/5)
@END
@t028
alpha = atan(5/12); sin(2x) = -1/2
x = 105, 165, 285, 345
@END
@t029
y' = x/y
y'' = 1/y^3
@END
@t030
1 + cos(2x) = 2cos(x)^2
1/2*tan(x) + C
@END
@t033
x = base + k*period
@END
@t032
sinA-sinB = 2cos((A+B)/2)sin((A-B)/2)
@END
@t034
sin^2+cos^2 = 1; /cos^2 if valid
@END
@t035
u = sin(x) or cos(x) or tan(x)
@END
@t036
R-form
@END
@t037
A = alpha or pi-alpha
@END
@t038
u = trig(x); quadratic in u
@END
@t039
trig(x) = k => angle families
@END
@t040
sin(A)=k or cos(A)=k or tan(A)=k
@END
@t041
A = alpha + kP
@END
@t042
lo <= x <= hi
@END
@t043
x = base + kP
@END
@t044
s^2 = 1-c^2; t^2 = 1/c^2-1
@END
@t045
c^2 = 1-s^2; t^2 = s^2/(1-s^2)
@END
@t046
s^2 = t^2/(1+t^2); c^2 = 1/(1+t^2)
@END
@t052

@END
@t057
(LHS)=d/dx(RHS)
@END
@t058
y'(...) = ...
@END
@t059
y' = (...)/(...)
@END
@t065
; dy/d
@END
@t073
dy/d
@END
@t074
d/dx(dy/dx)
@END
@t076
ax^2+bx+c = a(x+b/2a)^2+c-b^2/4a
@END
@t077
RHS = f
@END
@t079
coeff(LHS) = coeff(RHS)
@END
@t083
(a+b)^n = sum(nCr*a^(n-r)*b^r)
@END
@t084
x^r terms
@END
@t086
T_r = nCr*a^(n-r)*b^r
@END
@t088
s = x+y, p = xy
@END
@t089
x^2+y^2 = s^2-2p
@END
@t090
t^2-st+p = 0
@END
@t091
radicand >= 0
@END
@t092
sqrt(A) = B => A = B^2
@END
@t093
root in domain
@END
@t094
|A| = A or |A| = -A
@END
@t095
A = B; A = -B
@END
@t096
roots = union(valid)
@END
@t097
log args > 0; base > 0; base != 1
@END
@t098
log(a,A) = ln(A)/ln(a)
@END
@t099
log_b(A)-log_b(B) = log_b(A/B)
@END
@t100
u = a^x/e^x/log(x)
@END
@t101
u = ... => x = ...
@END
@t102
denominator != 0
@END
@t103
* common den
@END
@t104
reject denominator = 0
@END
@t105
(x-r1)(x-r2) = 0 or x = (-b+-sqrt(D))/(2a)
@END
@t106
root in domain
@END
@t108
den = factorised; PF
@END
@t109
PF: A/lin+(Bx+C)/q
@END
@t110
(x+h)^2+k => ln/atan
@END
@t111
sqrt(tan x) = (sin x + cos x)/sqrt(sin x cos x)
@END
@t112
u = sin(x)-cos(x)
@END
@t113
d/dx(xsin(x)+cos(x)) = xcos(x)
@END
@t114
* cos(x)/cos(x)
@END
@t115
I = u*v - Int(v du); cancel
@END
@t116
x -> a+b-x
@END
@t117
2I = Int(f(x)+f(a+b-x)) dx
@END
@t118
u = sqrt(1+x^n)
@END
@t119
u^2 = 1+x^n
@END
@t120
A/(u-1)+B/(u+1); x = ...
@END
@t121
x = a*sin(t) or a*tan(t) or a*sec(t)
@END
@t122
triangle: opp,adj,hyp
@END
@t123
sqrt arg >= 0; denom != 0
@END
@t124
u = log(...), dv = rest dx
@END
@t125
u = log; dv = rest dx
@END
@t126
P/Q = q+R/Q or PF
@END
@t127
Int x^n dx = x^(n+1)/(n+1); Int dx/x = ln|x|
@END
@t128
sin^2+cos^2 = 1; sin(2x) = 2sin(x)cos(x)
@END
@t129
u = sin(x), cos(x), or tan(x)
@END
@t130
Int f'(x) dx = f(x)
@END
@t131
du = u' dx; I = u*v - Int(v du); PF
@END
@t132
F(hi)-F(lo)
@END
@t133
+C
@END
@t134
Err: invalid input
@END
@t135
LHS = expanded
coeff(LHS) = coeff(RHS)
constants = ...
@END
@t136
dy/dx+P(x)y = Q(x)
IF=e^Int(Pdx)
y*IF=Int(Q*IF dx)+C
@END
@t137
m = dy/dx at x1
y-y1 = m(x-x1)
@END
@t138
LHS = ...
... = RHS
@END
@t139
v = u+at; s = ut+1/2*a*t^2; v^2 = u^2+2as
values => equation
unknown = ...
@END
@t140
@END
@t141
I = Int[f(x)] dx
du = u' dx; I = u*v - Int(v du); PF/id
d(ans)/dx = f(x)
@END
@t142
denom != 0; log arg > 0; sqrt arg >= 0
LHS - RHS = 0
roots ∩ domain
@END
@t143
X ~ B(n,p)
P(X<=r) or P(X>=r)
@END
@t144
LHS - RHS = 0
@END
@t145
(x-r1)(x-r2) = 0
@END
@t146
sub root
@END
@t147

@END
@t148
=
@END
@t149
d/dx(sin(x)) = cos(x)
@END
@t150
d/dx(-cos(x)) = sin(x)
@END
@t151
d/dx(ln(abs(x))) = 1/x
@END
@t152
d/dx(tan(x)) = sec(x)^2
@END
@t153
d/dx(sec(x)) = sec(x)tan(x)
@END
@t154
tan(x)^2 = sec(x)^2 - 1
@END
@t155
d/dx(-cot(x)) = cosec(x)^2
@END
@t156
d/dx(-cosec(x)) = cosec(x)cot(x)
@END
@t157
Integrate term by term.
@END
@t158
d/dx(e^x) = e^x
@END
@t159
Int k dx = kx
@END
@t000
Valid: all x
@END
@t001
Valid: |b/a|<1
@END
@t002
Valid: |x|<1
@END
@t003
Valid: |(
@END
@t080

@END
@t085
Coeff: 
@END
@t107
I = Int[
@END
@t064
Area = Int y*dx/d
@END
@t066
dy/dx = (dy/d
@END
@t007
= 
@END
@t008
u = 
@END
@t014
dv = 
@END
@t031

@END
@t047

@END
@t048
)=0
@END
@t049

@END
@t050
 only
@END
@t051
Ids: 
@END
@t053
d/dx: 
@END
@t054
 = 
@END
@t055
F = 
@END
@t060
x = 
@END
@t061
y = 
@END
@t062
dx/d
@END
@t063
=diff(x,
@END
@t056
d/d
@END
@t067
)/(dx/d
@END
@t068
d2y/dx2=(d/d
@END
@t069
(dy/dx))/(dx/d
@END
@t070
y = 
@END
@t071
u = 
@END
@t072
du/d
@END
@t075
f = 
@END
@t078

@END
@t081
Base = 
@END
@t082
Power = 
@END
@t087

@END
