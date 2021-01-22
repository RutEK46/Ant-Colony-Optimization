import matplotlib.pyplot as plt
import pandas
import sys

data = pandas.read_csv(sys.argv[1])
print(data)

i = 0
try:
    while True:
        row = data[str(i)]
        print(row)
        
        plt.plot(row)
        plt.legend([f"Item {i}"])
        plt.ylim([-0.1, 1.1])
        plt.savefig(f"{sys.argv[1]}_{i}.jpeg")
        plt.close()
        
        
        i += 1
except KeyError:
    pass