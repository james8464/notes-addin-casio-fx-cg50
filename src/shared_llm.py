"""
Shared LLM Interface for CASIO Test Suite - PC ONLY.

This module connects to Ollama for LLM verification.
DO NOT DEPLOY TO CALCULATOR - uses unsupported modules.
"""

# PC-ONLY GUARD
# This module is intentionally not compatible with the fx-CG50 runtime.
# On MicroPython (including Casio's port), fail fast on import.
import sys
_MICROPY = getattr(getattr(sys, "implementation", None), "name", "") == "micropython"
if _MICROPY:
    raise ImportError("PC-only module - not supported on MicroPython")

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

LLM_SYSTEM_PROMPT = """Check math working. Steps follow? no fake "hence"? rules ok? answer matches?
Equiv forms ok.

One word:
CORRECT — clear
INCORRECT — bad steps/answer
NEEDS_REVIEW — unsure"""


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
        """Create cache key; hash full prompt to avoid collision on long/ similar tails."""
        h = hashlib.sha256()
        h.update(model.encode("utf-8", errors="replace"))
        h.update(b"\0")
        h.update(prompt.encode("utf-8", errors="replace"))
        return h.hexdigest()
    
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
            "hit_rate": "{0:.1f}%".format(hit_rate)
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
    
    def verify(self, program_output, expected, context="", check_working_quality=False, stream_callback=None):
        """
        Verify program output using LLM.

        Args:
            program_output: The CASIO program output string
            expected: The expected answer string
            context: Additional context (question, method, etc.)
            check_working_quality: If True, check working quality not just answer
            stream_callback: Optional callback for streaming responses

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

        if check_working_quality:
            verdict, explanation = self._check_working_quality(program_output, stream_callback)
            return {
                "verdict": verdict,
                "explanation": explanation,
                "confidence": 0.95,
                "cached": False
            }

        prompt = self._build_prompt(program_output, expected, context)
        cached = self.cache.get(self.selected_model, prompt)
        if cached:
            cached["cached"] = True
            return cached

        try:
            result = self._query_ollama(prompt, stream_callback)
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
                "explanation": "LLM error: {0}".format(self.last_error),
                "confidence": 0,
                "cached": False
            }
    
    def _build_prompt(self, program_output, expected, context):
        """Build verification prompt."""
        return (
            "CTX: {0}\n\n"
            "OUT: {1}\n\n"
            "EXP: {2}\n\n"
            "{3}"
        ).format(context, program_output, expected, LLM_SYSTEM_PROMPT)
    
    def _query_ollama(self, prompt, stream_callback=None):
        """Send query to Ollama with optional streaming."""
        cmd = [
            "ollama", "run",
            self.selected_model,
            prompt
        ]

        if stream_callback:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            output_lines = []
            for line in iter(process.stdout.readline, ''):
                if line:
                    output_lines.append(line)
                    stream_callback(line)
                if not line and process.poll() is not None:
                    break
            process.wait()
            if process.returncode != 0:
                stderr = process.stderr.read()
                raise Exception("Ollama error: {0}".format(stderr))
            return ''.join(output_lines).strip()
        else:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=LLM_TIMEOUT_SECONDS
            )
            if result.returncode != 0:
                raise Exception("Ollama error: {0}".format(result.stderr))
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
    
    def _check_working_quality(self, program_output, stream_callback=None):
        """Check working quality by pattern matching."""
        if stream_callback:
            stream_callback("Checking working quality...\n")
        import re
        lines = program_output.strip().split('\n')

        working_lines = []
        use_lines = []
        calc_lines = []
        for line in lines:
            stripped = line.strip()

            if stripped.startswith('Method:'):
                continue
            if stripped.startswith('Answer:'):
                continue
            if 'Hence' in stripped:
                continue

            patterns = ['Let ', 'let ', 'dy/dx', 'du/dx', 'substitute', 'Using ', 'using ', 'Use ', 'use ']
            has_pattern = any(p in stripped for p in patterns)

            content = re.sub(r'^\d+\.\s*', '', stripped)

            if content.startswith('= '):
                has_pattern = True

            if has_pattern:
                if 'Use ' in stripped or 'use ' in stripped:
                    use_lines.append(stripped)
                else:
                    working_lines.append(stripped)
                    if any(p in content for p in ['Let ', 'dy/dx', 'du/dx', '=', 'substitute']):
                        calc_lines.append(stripped)

        all_working = working_lines + use_lines

        if len(all_working) == 0:
            return "INCORRECT", "No working steps shown (just Method/Hence/Answer)"

        if len(calc_lines) == 0 and len(use_lines) == 1:
            return "INCORRECT", f"Only identity shown, no calculation steps: '{use_lines[0][:50]}'"

        return "CORRECT", f"Working steps shown ({len(all_working)} lines)"
    
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

    def generate(self, prompt, stream_callback=None):
        """
        Generate a response from LLM for custom prompt.
        
        Args:
            prompt: The prompt to send to LLM
            stream_callback: Optional callback for streaming responses
        
        Returns:
            dict with keys: response, error, cached
        """
        if not self.enabled or not self.selected_model:
            return {
                "response": "",
                "error": "LLM not enabled or no model selected",
                "cached": False
            }
        
        try:
            result = self._query_ollama(prompt, stream_callback)
            return {
                "response": result.strip(),
                "error": "",
                "cached": False
            }
        except Exception as e:
            return {
                "response": "",
                "error": str(e),
                "cached": False
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
    print("Ollama available: {0}".format(available))
    
    if available:
        models = get_ollama_models()
        print("Found {0} models:".format(len(models)))
        for i, m in enumerate(models[:5]):
            print("  {0}. {1} ({2})".format(i + 1, m.get("name", "?"), m.get("size", "?")))
        
        print()
        print("Quick verification test...")
        
        result = quick_verify("sin(30)", "0.5", "Find sin(30)")
        print("Result: {0}".format(result))
    else:
        print("Ollama not detected. Install from https://ollama.ai")