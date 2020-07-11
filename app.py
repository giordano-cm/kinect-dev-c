import numpy as np
import matplotlib.pyplot as plt

data = np.genfromtxt("linear_scanner.csv", delimiter=",", names=["index", "depth_value", "angle", "direction"])
plt.plot(data['angle'], data['depth_value'])
plt.show()