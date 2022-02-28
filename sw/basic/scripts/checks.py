import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib import colors
from operator import sub
import os
import matplotlib.patches as mpatches
import glob
from statistics import mean
from statistics import stdev
import seaborn as sns
from scipy.stats import norm

################################################################################

log_dir = "./log/"

colors = ["red", "orange", "yellow", "green", 
          "blue", "purple", "grey", "black"]

################################################################################

def getMeasurements(filename):
  
  with open(filename) as textFile:
    t = [
      [l(s) for l,s in zip((float,int),line.split())] 
      for line in textFile
      ]
  
  fail_none = []
  fail_sw   = []
  for m in t:
    if(m[1]==0):
      fail_none.append(m[0])
    else:
      fail_sw.append(m[0])

  return [fail_none, fail_sw]

legends = []
def plotHistogram(f_none, f_sw, color, filename):
  
  legend = mpatches.Patch(color=color, label=filename[:-4])
  legends.append(legend)
  plt.legend(handles=legends, loc='upper right')

  ys = np.concatenate([f_none, f_sw])

  bins = np.linspace(0.0, 1.3, 130) # for vlabs
  # bins = np.linspace(0.0, 0.3, 130) # for flipflop

  # best fit of data
  (mu, sigma) = norm.fit(ys)

  # the histogram of the data
  n, nbins, patches = plt.hist(
    ys, 
    bins, 
    label='time', 
    color=color,
    histtype=u'step',
    alpha=0.25)

  # add a 'best fit' line
  y = norm.pdf(nbins, mu, sigma)
  plt.plot(nbins, y*30, color=color, linewidth=2)

  plt.tight_layout()
  plt.savefig(log_dir+'figure.png')

def statistics(f_none, f_sw, filename):
  
  # For all measurements
  m_all = f_none + f_sw
  
  print("For " + filename)
  print("  Fail Rate         " + str( (len(f_sw )/len(m_all)) * 100) + "%" )
  print("  Exec Time Mean    " + str(  mean(m_all)                 )       )
  print("  Exec Time StdDev  " + str(  stdev(m_all)                )       )
  
################################################################################

log_count = len(glob.glob1(log_dir, "*.log"))

f = 0
log_dir = "./log/"
for file in os.listdir(log_dir):
  if file.endswith(".log"):
    filepath = os.path.join(log_dir, file)
    [f_none, f_sw] = getMeasurements(filepath)
    plotHistogram(f_none, f_sw, colors[f], log_dir+file)
    statistics(f_none, f_sw, file)
    f += 1