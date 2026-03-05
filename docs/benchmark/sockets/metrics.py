import time
import statistics

lat = []

def record(t0):
    lat.append(time.perf_counter() - t0)

def report(name, ops, elapsed):

    print(f"\n{name}")
    print("-"*50)

    if not lat:
        print("NO DATA")
        return

    lat_sorted = sorted(lat)

    def pct(p):
        return lat_sorted[int(len(lat_sorted)*p)] * 1e6

    print("ops/sec:", int(ops/elapsed))
    print("avg us:", statistics.mean(lat_sorted)*1e6)
    print("p50 us:", pct(0.50))
    print("p99 us:", pct(0.99))
