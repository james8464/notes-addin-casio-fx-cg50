def cache_store(cache, key, value, limit):
    if limit <= 0:
        return value
    if key not in cache and len(cache) >= limit:
        trim = limit // 8
        if trim < 1:
            trim = 1
        i = 0
        while i < trim and cache:
            try:
                oldest = next(iter(cache))
                del cache[oldest]
            except (KeyError, StopIteration):
                break
            i += 1
    cache[key] = value
    return value


def enforce_total_cache_limit(caches, total_limit):
    if total_limit <= 0:
        return
    total = 0
    for cache in caches:
        total += len(cache)
    if total <= total_limit:
        return
    drop = total - total_limit
    if drop < total_limit // 8:
        drop = total_limit // 8
    if drop < 1:
        drop = 1
    for cache in caches:
        while drop > 0 and cache:
            try:
                oldest = next(iter(cache))
                del cache[oldest]
            except (KeyError, StopIteration):
                break
            drop -= 1
        if drop <= 0:
            return


def clear_all_caches(*caches):
    for cache in caches:
        if isinstance(cache, dict) and cache and all(hasattr(value, "clear") for value in cache.values()):
            for value in list(cache.values()):
                value.clear()
            continue
        cache.clear()
