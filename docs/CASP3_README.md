# CASP3.g3a README

## Purpose

`CASP3.g3a` is the Edexcel A-level Maths Paper 3 helper. It focuses on method lines, formula choice, substitutions, conclusions, and units. It is not intended to replace the fx-CG50 statistics menus for raw distribution lookup.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/CASP3.g3a`

## Controls

- Same console shell as KhiCAS.
- Type a command at the prompt, e.g. `suvat(u=2,a=3,t=4)`, then press `EXE`.
- Use the KhiCAS catalog/menu keys to insert commands; the CASP3 catalog contains only Paper 3 commands.
- History, cursor movement, delete, redraw, and scrolling follow the original KhiCAS controls.

## Mechanics Commands

### SUVAT

`suvat(known=value,known=value,known=value)`

Known symbols: `u`, `v`, `a`, `s`, `t`.

Examples:

- `suvat(u=2,a=3,t=4)` finds `v` and `s`.
- `suvat(u=5,v=17,t=3)` finds `a` and `s`.
- `suvat(u=4,v=10,a=2)` finds `s` and `t`.
- `suvat(s=40,u=5,t=4)` finds `a` and `v`.
- `suvat(v=20,a=3,s=50)` finds `u` and `t`.

Output shows selected SUVAT equation, substitution, solved value, and units when implied.

### Projectiles

`projectile(u,angle)`

`projectile(u,angle,g)`

`projectileh(u,angle,height)`

`projectileh(u,angle,height,g)`

`projectiley(u,angle,y,h0)`

`projectiley(u,angle,y,h0,g)`

`projectileat(u,angle,x,h0)`

`projectileat(u,angle,x,h0,g)`

`projectileangle(u,x,y,h0)`

`projectileangle(u,x,y,h0,g)`

Examples:

- `projectile(20,30)` range and flight time from level ground.
- `projectile(20,30,9.81)` same with custom `g`.
- `projectileh(20,30,5)` times at height 5 m.
- `projectiley(20,30,0,2)` hits ground from initial height 2 m.
- `projectileat(20,30,15,0)` height/speed at horizontal distance 15 m.
- `projectileangle(20,30,5,0)` launch angles to hit `(30,5)`.

### Forces

`force(m,a)`

`weight(m)`

`friction(mu,R)`

Examples:

- `force(12,3)` gives `F=ma`.
- `weight(6)` gives `W=mg`.
- `friction(0.4,25)` gives limiting friction.

### Inclines

`incline(m,angle,mu)`

`inclineacc(m,angle,mu)`

Examples:

- `incline(5,30,0.2)` resolves weight and friction on a plane.
- `inclineacc(5,30,0.2)` gives acceleration down/up the plane depending sign.

### Pulleys

`pulley(m1,m2)`

`pulley(m1,m2,mu)`

Examples:

- `pulley(3,5)` connected particles without friction.
- `pulley(3,5,0.2)` includes friction where supported.

### Moments

`beam(length,force,distance,weight)`

`moment(force,distance)`

Examples:

- `beam(10,30,4,20)` moment balance on a uniform beam.
- `moment(50,2.5)` single moment calculation.

### Variable Acceleration

`varacct(a,b,v0,t0,t)`

`varaccx(a,b,v0,x0,x)`

Examples:

- `varacct(6,-4,0,3,5)` integrates acceleration as a function of time.
- `varaccx(2,3,4,0,10)` uses `a = v dv/dx` where supported.

### Vector Motion

`vectorkin(x0,y0,vx,vy,ax,ay,t)`

Example:

- `vectorkin(0,0,3,4,0,-9.8,2)` vector position/velocity after 2 s.

## Statistics Commands

### Binomial

`binom(n,p,r)`

`binomtail(n,p,r,tail)`

`binomstats(n,p)`

`critbinom(n,p,alpha,tail)`

`binomcrit(n,p,alpha,tail)`

`binomnorm(n,p,lo,hi)`

Examples:

- `binom(10,0.4,3)` exact `P(X=3)`.
- `binomtail(10,0.4,3,-1)` lower tail `P(X<=3)`.
- `binomtail(10,0.4,3,1)` upper tail `P(X>=3)`.
- `binomstats(20,0.35)` mean and variance.
- `critbinom(20,0.4,0.05,-1)` lower critical region.
- `binomcrit(20,0.4,0.05,1)` upper critical region.
- `binomnorm(100,0.4,35,45)` normal approximation with continuity correction.

### Hypothesis Testing

`hypbinom(n,p,x,alpha,tail)`

`hyp_test(n,p,x,alpha,tail)`

Tail values:

- `-1`: lower-tail test.
- `1`: upper-tail test.
- `0`: two-tail style comparison where supported.

Examples:

- `hypbinom(20,0.4,4,0.05,-1)`
- `hypbinom(20,0.4,14,0.05,1)`
- `hypbinom(20,0.4,4,0.1,0)`

Output defines distribution, hypotheses, tail probability/critical comparison, and contextual decision.

### Normal Distribution

`normalprob(lo,hi,mu,sigma)`

`normalprobvar(lo,hi,mu,variance)`

`normaltail(x,mu,sigma,tail)`

`invnormal(p,mu,sigma)`

`normalcrit(p,mu,sigma)`

`samplemean(mu,sigma,n,lo,hi)`

`samplemeantail(mu,sigma,n,x,tail)`

`normalparams(mean,sd,n)`

Examples:

- `normalprob(40,60,50,10)` interval probability.
- `normalprobvar(40,60,50,100)` same using variance.
- `normaltail(65,50,10,1)` upper tail.
- `normaltail(35,50,10,-1)` lower tail.
- `invnormal(0.95,50,10)` inverse normal.
- `normalcrit(0.95,50,10)` same inverse normal critical-value route.
- `samplemean(50,10,25,45,55)` interval probability for `Xbar`.
- `samplemeantail(50,10,25,55,1)` upper-tail probability for `Xbar`.
- `normalparams(50,10,25)` sample mean standard error route.

### Probability

`cond(PAandB,PB)`

`probor(PA,PB,PAandB)`

`bayes(PBgivenA,PA,PB)`

`independent(PA,PB,PAandB)`

Examples:

- `cond(0.2,0.5)`
- `probor(0.4,0.3,0.1)`
- `bayes(0.8,0.3,0.5)`
- `independent(0.4,0.5,0.2)`

### Regression / Correlation

`regresscalc(n,sumx,sumy,sumx2,sumy2,sumxy)`

`pmcc(n,sumx,sumy,sumx2,sumy2,sumxy)`

`spearman(d2sum,n)`

Examples:

- `regresscalc(5,20,30,100,220,140)`
- `pmcc(5,20,30,100,220,140)`
- `spearman(12,8)`

### Grouped / Discrete Data

`groupmean(mid1,f1,mid2,f2,...)`

`groupmedian(L,cf_before,f,width,N)`

`histdensity(f,width)`

`meanvar(x1,p1,x2,p2,...)`

Examples:

- `groupmean(5,12,15,30,25,18)`
- `groupmedian(20,14,18,10,60)`
- `histdensity(24,6)`
- `meanvar(1,0.2,2,0.5,3,0.3)`

### Poisson

`poisson(lambda,r)`

`poissontail(lambda,r,tail)`

`poissonstats(lambda)`

Examples:

- `poisson(3,2)`
- `poissontail(3,2,-1)`
- `poissontail(3,2,1)`
- `poissonstats(4.5)`

## Command-Only Input

This app is command-only. It starts at an input prompt like KhiCAS: type the function and parameters, press `EXE`, then read the working lines.

## Full Command / Alias Index

Aliases call the same feature as their primary command.

- SUVAT: `suvat`
- Projectiles: `projectile`, `proj`, `projectiles`, `projectileh`, `projectileheight`, `projheight`, `projectiley`, `projectilelevel`, `projlevel`, `projectileat`, `projectilepoint`, `projat`, `projectileangle`, `projangle`, `targetangle`
- Basic mechanics: `force`, `newton`, `fma`, `weight`, `mg`, `friction`, `limitingfriction`, `moment`, `moments`
- Moments/statics: `beam`, `beamreactions`, `supportreactions`, `ladder`, `ladderwall`, `ladderrough`, `equilibrium`, `forcebalance`, `balanceforces`, `equilpolar`, `forcepolar`, `balancepolar`
- Inclines/pulleys: `incline`, `slope`, `plane`, `inclineacc`, `roughplaneacc`, `planeacc`, `connected`, `connectedparticles`, `twoparticles`, `pulley`, `pulleys`, `inclinepulley`, `pulleyincline`, `connectedincline`, `roughinclinepulley`, `roughpulleyincline`, `roughconnectedincline`
- Momentum/energy: `impulse`, `momentumchange`, `momentum`, `momcons`, `consmomentum`, `commonvelocity`, `coalesce`, `stick`, `work`, `workdone`, `power`, `powerrate`, `energy`, `kepe`, `workenergy`, `workenergyforce`, `energyforce`, `driveforce`, `restitution`, `impact`, `collision`, `impactsolve`, `collisionsolve`, `restitutionsolve`
- Variable/vector motion: `varacct`, `varacctpoly`, `variableacct`, `varaccx`, `varaccxpoly`, `variableaccx`, `varacc`, `variableacc`, `variableacceleration`, `vectorkin`, `vectormotion`, `vectorsuvat`, `vector`, `resultant`, `components`, `resolve`, `componentsfromforce`, `forcecomponents`
- Normal distribution: `normal`, `normalz`, `zscore`, `normalvar`, `normalzvar`, `zscorevar`, `normalprob`, `normalcdf`, `normint`, `normalprobvar`, `normalcdfvar`, `normintvar`, `normaltail`, `normalupper`, `normaltp`, `normaltailvar`, `normaluppervar`, `normaltpvar`, `normalcond`, `normalgiven`, `normalconditional`, `normalcondbetween`, `normalgivencap`, `normalconditionalbetween`, `invnormal`, `inversenormal`, `normalinv`, `normalcrit`, `invnormalvar`, `inversenormalvar`, `normalinvvar`, `normalparams`, `normalparameters`, `normalmeansd`
- Sample mean: `samplemean`, `xbarprob`, `samplemeanprob`, `samplemeantail`, `xbartail`, `sampletail`
- Binomial: `binom`, `binomial`, `binompdf`, `binomcdf`, `binomialcdf`, `bincdf`, `binomtail`, `binomialtail`, `bintail`, `binomstats`, `binommeanvar`, `binomialstats`, `binomparams`, `binomfrommeanvar`, `binomialparams`, `binomnorm`, `normalapproxbinom`, `binomnormal`, `largebinomnormal`, `binomapprox`, `poissonapprox`, `poissonapproxbinom`, `binompoisson`
- Hypothesis/critical regions: `critbinom`, `criticalbinom`, `criticalregion`, `binomcrit`, `hypbinom`, `binomtest`, `hypothesistest`, `hypnormal`, `normaltest`, `hypmean`, `critpoisson`, `criticalpoisson`, `poissoncrit`, `hyppoisson`, `poissontest`, `poissonhyp`
- Probability: `cond`, `conditional`, `given`, `probor`, `union`, `aorb`, `bayes`, `bayestheorem`, `reverseconditional`, `independent`, `independence`, `testindependent`, `prob_work`, `probwork`
- Poisson: `poisson`, `poissonpdf`, `poissoncdf`, `poissonle`, `poissontail`, `poissonrange`, `poissonbetween`, `porange`, `poissonstats`, `poissonmeanvar`, `postats`, `poissonnorm`, `normalapproxpoisson`, `poissonnormal`
- Regression/correlation: `regress`, `regression`, `predict`, `regresscalc`, `regressionline`, `lobf`, `regresss`, `regresssummary`, `regsummary`, `pmcc`, `correlation`, `productmoment`, `pmccs`, `correlations`, `productmoments`, `spearman`, `spearmanrank`, `rankcorr`, `corrtest`, `correlationtest`, `pmcctest`, `regress_work`, `regresswork`
- Data: `meanvar`, `variance`, `summary`, `groupmean`, `groupedmean`, `groupstats`, `discrete`, `expectation`, `randomvar`, `stratified`, `stratifiedsample`, `stratum`, `groupmedian`, `groupedmedian`, `interpolatemedian`, `groupquantile`, `groupedq`, `interpolateq`, `histdensity`, `frequencydensity`, `freqdensity`, `histfreq`, `histfrequency`, `frequencyfromdensity`, `outliers`, `iqrfences`, `outlierfences`
- Coding transforms: `code`, `codex`, `xtocoded`, `uncode`, `decodecoded`, `codedtox`
- Topic wrappers: `normal_work`, `normalwork`, `binom_work`, `binomwork`, `hyp_test`, `hyptest`
