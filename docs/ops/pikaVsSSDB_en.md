# Pika vs SSDB Performance Comparison

## Test Environment

Same configuration for both server and client machines (1 each):

    CPU: 24 Cores, Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
    MEM: 198319652 kB
    OS: CentOS release 6.2 (Final)
    NETWORK CARD: Intel Corporation I350 Gigabit Network Connection
 
## Test Interfaces
SET and GET

## Test Method

Using default configurations for both SSDB and Pika (Pika configured with 16 workers), the client executes:
```
./redis-benchmark -h ... -p ... -n 1000000000 -t set,get -r 10000000000 -c 120 -d 200
```
This performs 1 billion writes + 1 billion reads on both SSDB and Pika via the SET and GET interfaces.

## Result Charts
<img src="images/benchmarkVsSSDB01.png" height = "400" width = "480" alt="1">
<img src="images/benchmarkVsSSDB02.png" height = "400" width = "480" alt="10">

## Detailed Test Results (1 Billion)

### Set：

	ssdb
        1000000000 requests completed in 25098.81 seconds
		0.52% <= 1 milliseconds 
		96.39% <= 2 milliseconds 
		97.10% <= 3 milliseconds 
		97.34% <= 4 milliseconds 
		97.57% <= 5 milliseconds
		…
		98.87% <= 38 milliseconds
		...
		99.99% <= 137 milliseconds
		100.00% <= 215 milliseconds
		...
		100.00% <= 375 milliseconds
		
		39842.53 requests per second
	
	pika
        1000000000 requests completed in 11890.80 seconds
		18.09% <= 1 milliseconds
		93.32% <= 2 milliseconds
		99.71% <= 3 milliseconds
		99.86% <= 4 milliseconds
		99.92% <= 5 milliseconds
		99.94% <= 6 milliseconds
		99.96% <= 7 milliseconds
		99.97% <= 8 milliseconds
		99.97% <= 9 milliseconds
		99.98% <= 10 milliseconds
		99.98% <= 11 milliseconds
		99.99% <= 12 milliseconds
		...
		100.00% <= 19 milliseconds
		...
		100.00% <= 137 milliseconds
		
		84098.66 requests per second
		
### Get：

	ssdb
		1000000000 requests completed in 12744.41 seconds
        7.32% <= 1 milliseconds
		96.08% <= 2 milliseconds
		99.49% <= 3 milliseconds
		99.97% <= 4 milliseconds
		99.99% <= 5 milliseconds
		100.00% <= 6 milliseconds
		...
		100.00% <= 229 milliseconds
		
		78465.77 requests per second
	
	pika
        1000000000 requests completed in 9063.05 seconds
		84.97% <= 1 milliseconds
		99.76% <= 2 milliseconds
		99.99% <= 3 milliseconds
		100.00% <= 4 milliseconds
		...
		100.00% <= 33 milliseconds
		
		110338.10 requests per second
		
## Test Conclusions

Because Pika's design supports multi-threaded read/write (SSDB only supports 1 write thread with multiple read threads), in this test environment, Pika's write performance is 2.1x that of SSDB (QPS: 84098 vs 39842), and read performance is 1.4x that of SSDB (QPS: 110338 vs 78465).

SSDB write latency distribution: 0.52% within 1ms, 97.10% within 3ms. Pika write latency distribution: 18.32% within 1ms, 99.71% within 3ms. This shows that Pika's write latency is significantly better than SSDB for the same write volume (1 billion).

SSDB read latency distribution: 7.32% within 1ms, 99.49% within 3ms. Pika read latency distribution: 84.97% within 1ms, 99.99% within 3ms. This shows that Pika's read latency is also better than SSDB for the same read volume (1 billion).

Therefore, in this test environment, Pika outperforms SSDB in all aspects.

## Notes

1. SSDB's write performance started reasonably well (80k–90k QPS), but gradually dropped to around 30k as writes continued, staying there until the test ended. Pika's write speed was relatively stable throughout the test, maintaining 80k–90k.
2. These test results are for reference only. Before using Pika, it is best to conduct performance tests tailored to your specific use case.
