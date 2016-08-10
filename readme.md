DAG-Aware MIG Rewriting
====
DAG-aware rewriting is a fast greedy algorithm for circuit compression proposed for years [[1]](https://people.eecs.berkeley.edu/~alanmi/publications/2006/dac06_rwr.pdf).
In this package, we integrate this algorithm into Majority-Inverter Graphs (MIGs) and implement in [ABC](https://bitbucket.org/alanmi/abc) with commands "gen_graph", "aigtomigrw" and "rmigrw".

Compilation
====

    git clone https://github.com/popo55668/DAG-Aware-MIG-Rewriting.git
    make
