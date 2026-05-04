def cache_store(cache, key, value, limit):
    """Store one cache value and trim gently when the small-device limit is hit."""
    if limit <= 0:
        return value
    if key not in cache and len(cache) >= limit:
        # Drop a small slice instead of clearing the whole cache.  On the CASIO
        # target, keeping a warm cache matters, but memory spikes matter more.
        target = limit - (limit // 8)
        if target >= limit:
            target = limit - 1
        if target < 0:
            target = 0
        keys = list(cache.keys())
        i = 0
        while len(cache) > target and i < len(keys):
            try:
                del cache[keys[i]]
            except KeyError:
                pass
            i += 1
    cache[key] = value
    return value


def enforce_total_cache_limit(caches, total_limit):
    """Keep a group of independent caches under one shared memory budget."""
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
        keys = list(cache.keys())
        i = 0
        while drop > 0 and i < len(keys):
            try:
                del cache[keys[i]]
                drop -= 1
            except KeyError:
                pass
            i += 1
        if drop <= 0:
            return


def clear_all_caches(*caches):
    """Clear regular caches and nested per-name cache dictionaries."""
    for cache in caches:
        if isinstance(cache, dict) and cache and all(hasattr(value, "clear") for value in cache.values()):
            for value in list(cache.values()):
                value.clear()
            continue
        cache.clear()
