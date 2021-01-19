# Ant-Colony-Optimalization

To compile use mpicc ACO_knapsackMPI.c

To run use mpirun a.out capacty_file weight_file profit_file
e. g. mpirun -np 2 a.out p05_c.txt p05_w.txt p05_p.txt

On stdout program should output csv file. You can store data in file using Linux redirect
e. g. mpirun -np 2 a.out p05_c.txt p05_w.txt p05_p.txt > result.csv

Files p*_*.txt are takes from https://people.sc.fsu.edu/~jburkardt/datasets/knapsack_01/knapsack_01.html
Files p*_s.txt are modified to be more human readable.
