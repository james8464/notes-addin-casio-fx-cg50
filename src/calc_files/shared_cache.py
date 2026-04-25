# Minimal stub for calculator
def cache_store(cache, key, value, limit):
    cache[key] = value
    return value

def clear_all_caches(caches):
    for c in caches:
        c.clear()