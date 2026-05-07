@t004
...
@END
@t005

@END
@t006
;t=
@END
@t009
u=inner
@END
@t010
du=u' dx
@END
@t011
Int du
@END
@t012
sub back
@END
@t013
I=uv-Int(vdu)
@END
@t015
du=u'dx; v=Int dv.
@END
@t016
I=uv-Int(vdu)
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
u=inner
@END
@t021
(uv)' = u'v+uv'
@END
@t022
(u/v)' = (u'v-uv')/v^2
@END
@t023
ln y; diff; *y
@END
@t024
FP quotient
cos diff id=>-sin(x)
@END
@t025
[f(x+h)-f(x)]/h -> f'(x)
@END
@t026
d/dx rule -> simplify
@END
@t027
u=asin(x-1); 5u=pi.
x=1+sin(pi/5).
@END
@t028
alpha=atan(5/12); sin2x=-1/2.
x=105,165,285,345.
@END
@t029
y'=x/y.
y''=1/y^3.
@END
@t030
1+cos(2x)=2cos(x)^2.
1/2tanx+C.
@END
@t033
x=base+k*period
@END
@t032
sum-prod; angle eq
@END
@t034
sin2+cos2=1 or /cos2
@END
@t035
u=sin/cos/tan; solve
@END
@t036
R-form
@END
@t037
base angles
@END
@t038
id=>quadratic
@END
@t039
trig value=>angle
@END
@t040
isolate trig fn
@END
@t041
CAST/base angles
@END
@t042
interval; check
@END
@t043
period; check
@END
@t044
s2=1-c2; t2=1/c2-1.
@END
@t045
c2=1-s2; t2=s2/(1-s2).
@END
@t046
s2=t2/(1+t2); c2=1/(1+t2).
@END
@t052

@END
@t057
 both sides
@END
@t058
y'(...)=...
@END
@t059
y'=(...)/(...)
@END
@t065
,dy/d
@END
@t073
dy/d
@END
@t074
diff dy/dx again
@END
@t076
ax^2+bx+c=a(x+b/2a)^2+c-b^2/4a
@END
@t077
expand RHS=f
@END
@t079
coeff(LHS)=coeff(RHS)
@END
@t083
(a+b)^n=sum(nCr*a^(n-r)*b^r)
@END
@t084
collect x^r
@END
@t086
T_r=nCr*a^(n-r)*b^r
@END
@t088
s=x+y, p=xy
@END
@t089
x^2+y^2=s^2-2p
@END
@t090
t^2-st+p=0
@END
@t091
radicand>=0
@END
@t092
sqrt(A)=B=>A=B^2
@END
@t093
root in domain
@END
@t094
|A|=A or |A|=-A
@END
@t095
solve each case
@END
@t096
roots=union(valid)
@END
@t097
log args>0; base>0, base!=1
@END
@t098
log_a(A)=b=>A=a^b
@END
@t099
root in domain
@END
@t100
u=a^x/e^x/log(x)
@END
@t101
solve u, then x
@END
@t102
denominator!=0
@END
@t103
multiply by common den
@END
@t104
reject denominator=0
@END
@t105
factor/rearrange/quad formula
@END
@t106
root in domain
@END
@t108
den=(...)(...); PF
@END
@t109
PF: A/lin,(Bx+C)/q.
@END
@t110
complete square=>ln/atan
@END
@t111
sqrt(tan)->sin/cos.
@END
@t112
u=sin(x)-cos(x)
@END
@t113
d(xsinx+cosx)=xcosx
@END
@t114
multiply by cos(x)/cos(x)
@END
@t115
IBP; cancel factor
@END
@t116
x -> a+b-x
@END
@t117
2I = Int(f(x)+f(a+b-x))dx
@END
@t118
u=sqrt(1+x^n).
@END
@t119
u^2=1+x^n
@END
@t120
PF in u; sub back
@END
@t121
x=a*sin(t) or a*tan(t) or a*sec(t)
@END
@t122
triangle -> back-sub
@END
@t123
radicand>=0; denom!=0
@END
@t124
u=log(...), dv=rest dx
@END
@t125
u=log; dv=rest dx
@END
@t126
division/PF
@END
@t127
Int powers/logs
@END
@t128
sin^2+cos^2=1; sin(2x)=2sinxcosx
@END
@t129
u=sinx/cosx/tanx if matched
@END
@t130
std integral
@END
@t131
u-sub/IBP/PF if needed
@END
@t132
F(hi)-F(lo)
@END
@t133
+C
@END
@t134
valid syntax
@END
@t135
expand LHS
coeff(LHS)=coeff(RHS)
constants=...
@END
@t136
dy/dx+P(x)y=Q(x)
IF=e^Int(Pdx)
y*IF=Int(Q*IF dx)+C
@END
@t137
m=dy/dx at x1
y-y1=m(x-x1)
@END
@t138
LHS=...
...=RHS
@END
@t139
v=u+at; s=ut+1/2*a*t^2; v^2=u^2+2as
sub values
unknown=...
@END
@t140
@END
@t141
I=Int[f(x)]dx
u-sub/IBP/PF/identity
d(answer)/dx=f(x)
@END
@t142
domain restrictions
LHS-RHS=0
roots satisfying domain
@END
@t143
X~B(n,p)
P(X<=r) or P(X>=r)
@END
@t144
LHS-RHS=0
@END
@t145
factor/rearrange
@END
@t146
sub root
@END
@t147

@END
@t148
=
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
I=Int[
@END
@t064
Area=Int y*dx/d
@END
@t066
dy/dx=(dy/d
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
F= 
@END
@t060
x= 
@END
@t061
y= 
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
y= 
@END
@t071
u= 
@END
@t072
du/d
@END
@t075
f= 
@END
@t078
expand 
@END
@t081
Base= 
@END
@t082
Power= 
@END
@t087

@END
