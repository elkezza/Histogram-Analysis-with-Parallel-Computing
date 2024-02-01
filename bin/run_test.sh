#!/bin/bash
output_file="performance_for_all_versions_results.csv"
echo "version,N,time" > $output_file

# Array of N values
declare -a N_values=(10 80 160 320 640 1280 2560 5120)

# Loop over each version
for version in atomic single_mutex mutex_per_bucket best
do
  # Loop over each N value
  for N in "${N_values[@]}"
  do
    # Run the program and capture the execution time
    start_time=$(date +%s.%N)
    ./$version --num-threads 32 --N $N --sample-size 30000000 --print-level 0
    end_time=$(date +%s.%N)
    
    # Calculate elapsed time
    elapsed=$(echo "$end_time - $start_time" | bc)
    
    # Log the results
    echo "$version,$N,$elapsed" >> $output_file
  done
done

