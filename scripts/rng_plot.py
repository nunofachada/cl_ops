import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
import re
import glob
import os

os.chdir('.')
files = glob.glob('out*.tsv')
cols = 3
rows = len(files) / cols + 1
print rows, cols

current = 1
plt.figure(1)
for f in files:
    t = re.split('_|\.', f)
    img = np.loadtxt(f, dtype=np.uint32)
    plt.subplot(rows,cols,current)
    imgplot = plt.imshow(img)
    plt.title(t[1] + ', ' + t[2] + ' seeds (' + t[3] + ')')
    current+=1

plt.show()


