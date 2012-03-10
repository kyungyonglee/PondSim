#!/usr/bin/env python

import sys, os, time
index = 0
time.sleep(1)
all_comb_latency = {}
all_comb_count = {}
for file in sys.argv:
  if index != 0:
    f_handler = open(file, 'r')
    latency_buf = []
    count_buf = []
    for line in f_handler:
      line_sep = line.split()
      count_buf.append(long(line_sep[0]))
      latency_buf.append(long(line_sep[1]))
    latency_buf.sort()
#    count_buf.sort()
    all_comb_latency[file] = latency_buf
  index = index +1

combined_file = open("col_combine", "w")
list_size = 0
for k,v in all_comb_latency.iteritems():
  combined_file.write(k+" ")
  list_size = len(v)

for i in range(0,list_size):
  combined_file.write("\n") 
  for k,v in all_comb_latency.iteritems():
    combined_file.write(str(v[i]) + "	")
