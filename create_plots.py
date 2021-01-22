import matplotlib.pyplot as plt
import pandas
import sys

data = pandas.read_csv(sys.argv[1])
print(data)


max_iteration = int(sys.argv[3]) if len(sys.argv) == 4 else float('inf')

i = 0
try:
    while True:
        row = data[str(i)]
        row = row[:int(min(len(row), max_iteration))]
        print(row)
        
        plt.plot(row)
        plt.legend([f"Item {i}"])
        plt.title(f"{sys.argv[2]} - Item {i}")
        plt.xlabel("Iteration")
        plt.ylabel("Is in Knapsack?")
        plt.ylim([-0.1, 1.1])
        plt.savefig(f"{sys.argv[2]}_{i}.jpeg")
        # plt.show()
        plt.close()
        
        
        i += 1
except KeyError:
    pass
    
for label in ['Profit', 'Weight']:
    row = data[label]
    row = row[:min(len(row), max_iteration)]
    print(row)

    plt.plot(row)
    plt.legend([label])
    plt.title(f"{sys.argv[2]} - {label}")
    plt.xlabel("Iteration")
    plt.ylabel(label)
    plt.savefig(f"{sys.argv[2]}_{label}.jpeg")
    # plt.show()
    plt.close()
