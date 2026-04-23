# CASIO LLM Enhancement Specification

## Overview

Adding local LLM-based verification to the CASIO test suite using Ollama. This enables multi-method verification of test outputs using SymPy calculations, CASIO program outputs, and LLM assessment.

---

## User Experience

### Command: `/llm`

When user runs `/llm`:
```
Available Ollama Models:
1. llama3.2 (3B params)
2. qwen2.5:3b (3B params)
3. mistral (7B params)
4. phi4 (4B params)

Enter number to select model (or 'q' to quit): _
```

### New Test Commands

```
/random 100                    # Run tests with SymPy only (existing)
/random 100 llm               # Run tests with SymPy + LLM verification
/random 100 llm+             # Run tests with SymPy + LLM + LLM-generated cases
```

---

## Implementation Architecture

### 1. Ollama Detection & Model Listing

```python
# Check if Ollama is installed and running
def check_ollama_available():
    try:
        result = subprocess.run(
            ['ollama', 'list'],
            capture_output=True,
            timeout=5
        )
        return result.returncode == 0
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False
```

### 2. Model Selection

```python
class LLMManager:
    def __init__(self):
        self.available_models = []
        self.selected_model = None
        self.enabled = False
    
    def list_models(self):
        """Parse 'ollama list' output"""
        result = subprocess.run(['ollama', 'list'], capture_output=True, text=True)
        # Parse header, extract model names
        # Store in self.available_models
    
    def select_model(self, index):
        """Select model by index"""
        self.selected_model = self.available_models[index]
    
    def test_connection(self):
        """Quick test with simple prompt"""
        # Test with "What is 2+2?" to verify model works
```

### 3. Three-Way Verification

For each test case, run three verifications in parallel:

```python
async def verify_with_consensus(program_output, expected, context):
    """
    Run three verification methods:
    1. SymPy calculation
    2. CASIO program output
    3. LLM assessment
    
    Return PASS if >= 2 methods agree
    """
    # Run all three in parallel using asyncio.gather
    sympy_result = await run_sympy_check(expected, context)
    casio_result = verify_casio_output(program_output, expected)
    llm_result = await run_llm_assessment(program_output, expected, context)
    
    results = [sympy_result, casio_result, llm_result]
    passing = sum(1 for r in results if r == 'CORRECT')
    
    return 'PASS' if passing >= 2 else 'FAIL'
```

### 4. SymPy Verification

```python
def run_sympy_check(expected, context):
    """Parse expected value and verify mathematically"""
    try:
        # Parse expression using SymPy
        expected_sp = SP_PARSE_EXPR(expected)
        
        # If program output is numeric, compare numerically
        # If symbolic, compare symbolically
        
        return 'CORRECT'  # or 'UNABLE_TO_VERIFY'
    except Exception:
        return 'ERROR'
```

### 5. LLM Prompt Template

```python
LLM_ASSESSMENT_PROMPT = """You are a math exam checker. 

QUESTION: {question}
PROGRAM OUTPUT: {program_output}
EXPECTED: {expected}

Context: {context}

Determine if PROGRAM OUTPUT is CORRECT for the given QUESTION.
Consider:
- Mathematical equivalence
- Alternative forms (e.g., sin²x = 1-cos²x)
- Domain restrictions
- Equivalent expressions

Respond with EXACTLY one word:
CORRECT - if the output is mathematically correct
INCORRECT - if the output is wrong
NEEDS_REVIEW - if you cannot determine
"""
```

### 6. Async Parallel Execution

```python
async def run_parallel_verification(test_case):
    """Run SymPy and LLM concurrently"""
    async with asyncio.TaskGroup() as tg:
        sympy_task = tg.create_task(run_sympy_async(test_case))
        llm_task = tg.create_task(run_llm_async(test_case))
    
    return sympy_task.result(), llm_task.result()
```

---

## Data Structures

### Test Result Record

```python
@dataclass
class LLMCapableTestRecord(TestRecord):
    sympy_verdict: str = ""      # CORRECT/INCORRECT/ERROR
    llm_verdict: str = ""        # CORRECT/INCORRECT/TIMEOUT
    consensus: str = ""         # PASS/FAIL
    sympy_explanation: str = ""
    llm_explanation: str = ""
```

### Configuration

```python
# Stored in test config or user preferences
LLM_CONFIG = {
    'enabled': False,
    'model': None,
    'timeout_seconds': 30,
    'parallel': True,
    'cache_responses': True
}
```

---

## UI/Output Integration

### Enhanced Test Results Display

```
=== TEST 4523 ===
Input: sin(x) = 0.5
       Degrees: 0 to 360
Program Output: x = 30, 150, 270
SymPy: x = 30° + 360°n or x = 150° + 360°n
LLM: CORRECT - Has two solutions in range
Verdict: PASS (3/3 consensus)
```

### Failed Test Report Additions

```
=== LLM FAILURE REPORT ===
Test 1243: sin(2x) = cos(x)
SymPy: PASS (2 solutions found)
CASIO: PASS (x = 90, 210)
LLM: INCORRECT - Missing solutions x = 90, 210
Warning: LLM may have missed domain restrictions
```

---

## Performance Optimization

### Caching

```python
class LLMResponseCache:
    """Cache LLM responses for repeated queries"""
    def __init__(self, max_size=1000):
        self.cache = {}
        self.access_times = {}
    
    def make_key(self, model, prompt):
        return hash((model, prompt[:100]))
    
    def get(self, model, prompt):
        key = self.make_key(model, prompt)
        return self.cache.get(key)
    
    def set(self, model, prompt, response, ttl_seconds=3600):
        key = self.make_key(model, prompt)
        # Implement TTL-based expiry
```

### Concurrent Execution

```python
class VerificationExecutor:
    def __init__(self, max_workers=4):
        self.executor = ThreadPoolExecutor(max_workers=max_workers)
    
    def verify_batch(self, test_cases):
        futures = [
            self.executor.submit(self.verify_single, tc)
            for tc in test_cases
        ]
        return [f.result() for f in futures]
```

---

## Error Handling

### Ollama Not Installed

When user runs `/llm` but Ollama is not installed:
```
Ollama Not Found
================
Ollama is required for LLM features. Install from: https://ollama.ai

Quick install:
  curl -fsSL https://ollama.ai/install.sh | sh

After installation, run:
  ollama serve

Then return here and run /llm again
```

### Model Not Available

When selected model is deleted:
```
Model 'llama3.2' not found
Available: qwen2.5:3b, mistral

Select: _
```

### LLM Timeout

When LLM takes > 30 seconds:
```
Test 1243: TIMEOUT
Fallback: Using SymPy only verification
Result: PASS
```

---

## Configuration File

Store preferences in CASIO config:

```python
# ~/.casiorc or similar
[llm]
enabled = true
model = qwen2.5:3b
timeout = 30
parallel = true
cache = true

[testing]
prefer_llm_if_available = false
consensus_threshold = 2
```

---

## Testing Protocol

### Phase 1: Basic
- Detect Ollama installation
- List available models
- Select model and save preference

### Phase 2: Verification
- Run /random 100 with LLM disabled (baseline)
- Run /random 100 with LLM enabled
- Compare pass rates

### Phase 3: Edge Cases
- LLM timeout handling
- Invalid model selection
- Cache effectiveness

---

## Future Enhancements

### Test Generation with LLM
```
# Optional: Generate tests with LLM
/random 100 llm+

LLM generates new test cases:
- Different from existing patterns
- Tests edge cases
- Mathematical variety ensured
```

### Step-by-Step Verification
```
/verify-step-by-step test_id

LLM validates each step of working:
Step 1: CORRECT - Method stated
Step 2: CORRECT - Identity applied
Step 3: CORRECT - Algebra correct
Step 4: CORRECT - Final answer
```

---

## Implementation Priority

1. **P0 - Core**
   - Detect Ollama availability
   - Model listing/selection UI
   - Basic LLM query response

2. **P1 - Verification**
   - SymPy comparison
   - CASIO output parsing
   - Consensus algorithm

3. **P2 - Performance**
   - Async execution
   - Response caching
   - Timeout handling

4. **P3 - Polish**
   - Enhanced result display
   - Failed test reporting
   - Configuration persistence

---

## Files to Modify

1. `tests/run_tests.py` - Add /llm command, LLM manager class
2. `tests/test_case_generation.py` - New file for LLM test generation
3. `src/shared_quality.py` - Add verification helpers
4. `src/shared_llm.py` - New file for Ollama interface
5. Documentation updates in docs/

---

*Specification Version: 1.0*
*Target: Implementation Ready*