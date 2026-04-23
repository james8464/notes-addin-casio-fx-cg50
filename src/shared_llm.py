"""
Shared LLM Interface for CASIO Test Suite.

Provides Ollama-based verification for math problem solutions.
Uses local LLM models via Ollama for assessment.

Requires:
- Ollama installed and running
- At least one model downloaded

Usage:
    from shared_llm import LLMManager, check_ollama_available
    
    manager = LLMManager()
    if manager.is_available():
        models = manager.list_models()
        manager.select_model(0)
        result = manager.verify("sin(30)", "0.5", "exact value")
"""

import subprocess
import time
import hashlib
import os
from datetime import datetime

try:
    import sys
    from pathlib import Path
    
    _ROOT = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(_ROOT))
    from shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    REASONING_MARKERS = ("method:", "use ", "let ", "solve ", "answer:")


LLM_TIMEOUT_SECONDS = 120
LLM_CACHE_MAX_SIZE = 1000
LLM_CACHE_TTL_SECONDS = 3600

LLM_SYSTEM_PROMPT = """Check math working quality. Verify:
- Steps logically follow
- No skipped steps ("hence" without explanation)
- Rules applied correctly
- Final answer follows from working
- Equivalent answer forms accepted

Reply ONE word:
CORRECT - Working is clear and logical
INCORRECT - Working has quality issues
NEEDS_REVIEW - Cannot assess"""


def check_ollama_available():
    """Check if Ollama is installed and a server is running."""
    try:
        result = subprocess.run(
            ["ollama", "list"],
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.returncode == 0
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


def get_ollama_models():
    """Get list of available Ollama models."""
    if not check_ollama_available():
        return []
    
    try:
        result = subprocess.run(
            ["ollama", "list"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode != 0:
            return []
        
        models = []
        lines = result.stdout.strip().split("\n")
        for line in lines[1:]:
            if line.strip():
                parts = line.split()
                if parts:
                    name = parts[0]
                    if name and name != "NAME":
                        size = parts[1] if len(parts) > 1 else "?"
                        models.append({
                            "name": name,
                            "size": size,
                            "full": line.strip()
                        })
        return models
    except Exception:
        return []


class LLMCache:
    """Simple TTL-based cache for LLM responses."""
    
    def __init__(self, max_size=LLM_CACHE_MAX_SIZE, ttl=LLM_CACHE_TTL_SECONDS):
        self.cache = {}
        self.access_times = {}
        self.ttl = ttl
        self.max_size = max_size
        self.hits = 0
        self.misses = 0
    
    def _make_key(self, model, prompt):
        """Create cache key from model and prompt."""
        short_prompt = prompt[:200]
        return hashlib.md5(f"{model}:{short_prompt}".encode()).hexdigest()
    
    def get(self, model, prompt):
        """Get cached response if exists and not expired."""
        key = self._make_key(model, prompt)
        
        if key in self.cache:
            stored_time, response = self.cache[key]
            if time.time() - stored_time < self.ttl:
                self.hits += 1
                return response
            else:
                del self.cache[key]
                if key in self.access_times:
                    del self.access_times[key]
        
        self.misses += 1
        return None
    
    def set(self, model, prompt, response):
        """Store response in cache."""
        if len(self.cache) >= self.max_size:
            oldest_key = min(self.access_times.keys(), 
                           key=lambda k: self.access_times[k])
            del self.cache[oldest_key]
            if oldest_key in self.access_times:
                del self.access_times[oldest_key]
        
        key = self._make_key(model, prompt)
        self.cache[key] = (time.time(), response)
        self.access_times[key] = time.time()
    
    def stats(self):
        """Return cache statistics."""
        total = self.hits + self.misses
        hit_rate = (self.hits / total * 100) if total > 0 else 0
        return {
            "size": len(self.cache),
            "hits": self.hits,
            "misses": self.misses,
            "hit_rate": f"{hit_rate:.1f}%"
        }


class LLMManager:
    """
    Manages Ollama LLM for math problem verification.
    
    Usage:
        manager = LLMManager()
        if manager.is_available():
            models = manager.list_models()
            manager.select_model(0)
            result = manager.verify(program_output, expected, context)
    """
    
    def __init__(self):
        self.available = check_ollama_available()
        self.models = []
        self.selected_model = None
        self.cache = LLMCache()
        self.enabled = False
        self.last_error = None
        
        if self.available:
            self.refresh_models()
    
    def is_available(self):
        """Check if Ollama is running."""
        self.available = check_ollama_available()
        return self.available
    
    def refresh_models(self):
        """Refresh list of available models."""
        self.models = get_ollama_models()
        return len(self.models) > 0
    
    def list_models(self):
        """Get list of available models with info."""
        if not self.available:
            self.refresh_models()
        return list(self.models)
    
    def select_model(self, index_or_name):
        """Select model by index or name."""
        if isinstance(index_or_name, int):
            if 0 <= index_or_name < len(self.models):
                self.selected_model = self.models[index_or_name]["name"]
                return True
        else:
            for i, m in enumerate(self.models):
                if m["name"] == index_or_name or index_or_name in m["name"]:
                    self.selected_model = m["name"]
                    return True
        
        return False
    
    def enable(self):
        """Enable LLM verification."""
        if self.selected_model and self.available:
            self.enabled = True
            return True
        return False
    
    def disable(self):
        """Disable LLM verification."""
        self.enabled = False
    
    def verify(self, program_output, expected, context=""):
        """
        Verify program output using LLM.
        
        Args:
            program_output: The CASIO program output string
            expected: The expected answer string
            context: Additional context (question, method, etc.)
        
        Returns:
            dict with keys: verdict, explanation, confidence, cached
        """
        if not self.enabled or not self.selected_model:
            return {
                "verdict": "DISABLED",
                "explanation": "LLM not enabled",
                "confidence": 0,
                "cached": False
            }
        
        prompt = self._build_prompt(program_output, expected, context)
        
        cached = self.cache.get(self.selected_model, prompt)
        if cached:
            cached["cached"] = True
            return cached
        
        try:
            result = self._query_ollama(prompt)
            response = self._parse_response(result)
            
            output = {
                "verdict": response.get("verdict", "ERROR"),
                "explanation": response.get("explanation", ""),
                "confidence": response.get("confidence", 0),
                "cached": False
            }
            
            if output["verdict"] in ("CORRECT", "INCORRECT", "NEEDS_REVIEW"):
                self.cache.set(self.selected_model, prompt, output)
            
            return output
            
        except Exception as e:
            self.last_error = str(e)
            return {
                "verdict": "ERROR",
                "explanation": f"LLM error: {self.last_error}",
                "confidence": 0,
                "cached": False
            }
    
    def _build_prompt(self, program_output, expected, context):
        """Build verification prompt."""
        return f"""MATH PROBLEM: {context}

PROGRAM OUTPUT: {program_output}

EXPECTED ANSWER: {expected}

{LLM_SYSTEM_PROMPT}"""
    
    def _query_ollama(self, prompt):
        """Send query to Ollama."""
        cmd = [
            "ollama", "run",
            self.selected_model,
            prompt
        ]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=LLM_TIMEOUT_SECONDS
        )
        
        if result.returncode != 0:
            raise Exception(f"Ollama error: {result.stderr}")
        
        return result.stdout.strip()
    
    def _parse_response(self, response_text):
        """Parse LLM response into structured data."""
        lines = response_text.strip().split("\n")
        
        first_line = lines[0].strip().upper()
        
        if "CORRECT" in first_line and "INCORRECT" not in first_line:
            verdict = "CORRECT"
            explanation = " ".join(lines[1:]) if len(lines) > 1 else ""
            confidence = 0.95
        elif "INCORRECT" in first_line:
            verdict = "INCORRECT"
            explanation = " ".join(lines[1:]) if len(lines) > 1 else ""
            confidence = 0.90
        elif "NEEDS_REVIEW" in first_line:
            verdict = "NEEDS_REVIEW"
            explanation = response_text
            confidence = 0.50
        else:
            verdict = "UNPARSED"
            explanation = response_text[:200]
            confidence = 0.30
        
        return {
            "verdict": verdict,
            "explanation": explanation.strip()[:500],
            "confidence": confidence
        }
    
    def get_status(self):
        """Get current status of LLM manager."""
        return {
            "available": self.available,
            "enabled": self.enabled,
            "selected_model": self.selected_model,
            "model_count": len(self.models),
            "cache_stats": self.cache.stats() if self.enabled else {},
            "last_error": self.last_error
        }


def quick_verify(program_output, expected, context=""):
    """
    Quick one-shot verification without managing manager state.
    
    Args:
        program_output: CASIO output
        expected: Expected answer
        context: Problem context
    
    Returns:
        dict with verification result
    """
    manager = LLMManager()
    if not manager.is_available():
        return {
            "verdict": "OLLAMA_NOT_AVAILABLE",
            "explanation": "Ollama not installed or not running",
            "models": []
        }
    
    models = manager.list_models()
    if not models:
        return {
            "verdict": "NO_MODELS",
            "explanation": "No Ollama models found",
            "models": []
        }
    
    manager.select_model(0)
    manager.enable()
    
    return manager.verify(program_output, expected, context)


if __name__ == "__main__":
    print("CASIO LLM Interface Test")
    print("=" * 40)
    
    available = check_ollama_available()
    print(f"Ollama available: {available}")
    
    if available:
        models = get_ollama_models()
        print(f"Found {len(models)} models:")
        for i, m in enumerate(models[:5]):
            print(f"  {i+1}. {m['name']} ({m['size']})")
        
        print()
        print("Quick verification test...")
        
        result = quick_verify("sin(30)", "0.5", "Find sin(30)")
        print(f"Result: {result}")
    else:
        print("Ollama not detected. Install from https://ollama.ai")