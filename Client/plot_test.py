from matplotlib import pyplot as plt
import numpy as np

with open("test.csv", "r") as f:
    data = [int(i) for i in f.read().split(",")[:-1]]

plt.plot(data)
plt.show()