# Upstream KhiCASen Import

Source page: https://www-fourier.univ-grenoble-alpes.fr/~parisse/casio/

Selected inputs:

- Source archive: `giac90_1addin.tgz`
- Source SHA256: `0e7d01b0de6bf75ea8ae732c60c84ae9e291c4811b2461795e5c93e24d1621de`
- Working reference binary: `khicasen.g3a`
- Reference binary SHA256: `e926c9a8e3111c51786e444f2b6f59104362c960925fd0704668f73a8987bfdf`

Reason:

- User explicitly requested `khicasen`.
- `khicasen.g3a` is monolithic and does not need `.ac2`.
- The previous `khicas50.g3a + khicas50.ac2` split package was the crash-risk path.

Local source mirror:

- `/Users/james/Developer/CASIO/khicas/upstream/giac90_1addin`
- Matches extracted `giac90_1addin.tgz` source except generated empty `testv.882`.
- Keeps upstream `iostream -> iostream.new` symlink.
