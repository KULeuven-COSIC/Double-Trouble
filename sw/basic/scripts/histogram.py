import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

with open("./log/measurements.log") as textFile:
  t = [map(int, line.split()) for line in textFile]

bins = np.linspace(0, 360, 120)

plt.rcParams.update({'font.size': 12})
plt.rcParams["figure.figsize"] = [16,3]
plt.hist(
  [list(t[5]), list(t[4]), list(t[1]), list(t[13]), list(t[10]), list(t[7])], 
  bins, 
  label=['SW_RD_L1', 'SW_RD_LLC', 'SW_RD_RAM', 'HW_RD_L2', 'HW_RD_LLC', 'HW_RD_RAM'])
plt.legend(loc='upper left')
plt.savefig('./log/histogram.png', dpi=360, bbox_inches='tight')
# plt.title('My title')
# plt.clf()